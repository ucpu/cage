#include <cage-core/timer.h>
#include <cage-core/hashString.h>
#include <cage-engine/opengl.h>

#include "processor.h"

#include <map>
#include <set>
#include <vector>
#include <string>

namespace
{
	std::map<String, std::string> codes;
	std::map<String, String> defines;
	std::set<String, StringComparatorFast> onces;

	ConfigBool configShaderPrint("cage-asset-processor/shader/preview");

	bool validDefineChar(const char c)
	{
		if (c >= 'a' && c <= 'z')
			return true;
		if (c >= 'A' && c <= 'Z')
			return true;
		if (c >= '0' && c <= '9')
			return true;
		if (c == '_')
			return true;
		return false;
	}

	bool validDefine(const String &s)
	{
		if (s.empty())
			return false;
		for (uint32 i = 0, len = s.length(); i < len; i++)
		{
			if (!validDefineChar(s[i]))
				return false;
		}
		return true;
	}

	String outputTokenization(String input)
	{
		String result;
		while (!input.empty())
		{
			uint32 v = 0;
			for (; v < input.length(); v++)
				if (!validDefineChar(input[v]))
					break;
			if (v > 0)
			{
				String token = subString(input, 0, v);
				input = remove(input, 0, v);
				if (toUpper(token) == token)
				{
					if (defines.find(token) != defines.end())
						token = defines[token];
				}
				result += token;
			}
			else
			{
				result += String({ &input[0], &input[0] + 1 });
				input = remove(input, 0, 1);
			}
		}
		return result;
	}

	bool evalExpToBool(const String &l)
	{
		if (isReal(l))
			return toFloat(l) != 0;
		else if (isBool(l))
			return toBool(l);
		else
		{
			CAGE_LOG_THROW(String("expression: ") + l);
			CAGE_THROW_ERROR(Exception, "expression cannot be converted to bool");
		}
	}

	String eval(const String &input);

	String evalExp(const String &l)
	{
		if (l.empty())
			CAGE_THROW_ERROR(Exception, "unexpected end of line");
		uint32 p = find(l, '|');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			return Stringizer() + (evalExpToBool(left) || evalExpToBool(right));
		}
		p = find(l, '&');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			return Stringizer() + (evalExpToBool(left) && evalExpToBool(right));
		}
		p = find(l, '<');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			if (isReal(left) && isReal(right))
				return Stringizer() + (toFloat(left) < toFloat(right));
			else
				return Stringizer() + (left < right);
		}
		p = find(l, '>');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			if (isReal(left) && isReal(right))
				return Stringizer() + (toFloat(left) > toFloat(right));
			else
				return Stringizer() + (left > right);
		}
		p = find(l, '=');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			if (isInteger(left) && isInteger(right))
				return Stringizer() + (toSint32(left) == toSint32(right));
			else
				return Stringizer() + (left == right);
		}
		p = find(l, '-');
		if (p != m)
		{
			sint32 left = toSint32(evalExp(subString(l, 0, p)));
			sint32 right = toSint32(evalExp(subString(l, p + 1, m)));
			return Stringizer() + (left - right);
		}
		p = find(l, '+');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			if (isInteger(left) && isInteger(right))
				return Stringizer() + (toSint32(left) + toSint32(right));
			else
				return left + right;
		}
		p = find(l, '%');
		if (p != m)
		{
			sint32 left = toSint32(evalExp(subString(l, 0, p)));
			sint32 right = toSint32(evalExp(subString(l, p + 1, m)));
			return Stringizer() + (left % right);
		}
		p = find(l, '/');
		if (p != m)
		{
			sint32 left = toSint32(evalExp(subString(l, 0, p)));
			sint32 right = toSint32(evalExp(subString(l, p + 1, m)));
			return Stringizer() + (left / right);
		}
		p = find(l, '*');
		if (p != m)
		{
			sint32 left = toSint32(evalExp(subString(l, 0, p)));
			sint32 right = toSint32(evalExp(subString(l, p + 1, m)));
			return Stringizer() + (left * right);
		}
		p = find(l, '^');
		if (p != m)
		{
			String left = evalExp(subString(l, 0, p));
			String right = evalExp(subString(l, p + 1, m));
			if (isDigitsOnly(right) && !right.empty())
			{
				uint32 index = toUint32(right);
				if (index < left.length())
					return String({ &left[index], &left[index] + 1 });
			}
			{
				CAGE_LOG_THROW(Stringizer() + "expression: '" + l + "'");
				CAGE_LOG_THROW(Stringizer() + "string: '" + left + "'");
				CAGE_LOG_THROW(Stringizer() + "index: '" + right + "'");
				CAGE_THROW_ERROR(Exception, "non integer index or out of bounds");
			}
		}
		if (l[0] == '!')
		{
			return Stringizer() + !evalExpToBool(evalExp(subString(l, 1, m)));
		}
		if (defines.count(l))
		{
			return defines[l];
		}
		return l;
	}

	String eval(const String &input)
	{
		String l = input;
		l = replace(replace(replace(l, " ", ""), "\t", ""), "\n", "");
		while (true)
		{
			uint32 z = find(l, ')');
			if (z == m)
				break;
			uint32 o = find(reverse(subString(l, 0, z)), '(');
			if (o == m)
			{
				CAGE_LOG_THROW(Stringizer() + "expression: '" + input + "'");
				CAGE_THROW_ERROR(Exception, "unmatched ')'");
			}
			l = replace(l, z - o - 1, o + 2, evalExp(subString(l, z - o, o)));
		}
		if (find(l, '(') != m)
		{
			CAGE_LOG_THROW(Stringizer() + "expression: '" + input + "'");
			CAGE_THROW_ERROR(Exception, "unmatched '('");
		}
		return evalExp(l);
	}

	void output(const String &s)
	{
		if (defines["shader"].empty())
		{
			if (!s.empty())
			{
				CAGE_LOG_DEBUG(SeverityEnum::Warning, logComponentName, Stringizer() + "output to unspecified shader: '" + s + "'");
			}
			return;
		}
		codes[defines["shader"]] += std::string(s.c_str(), s.length()) + "\n";
	}

	uint32 shaderType(const String &name)
	{
		if (name == "vertex")
			return GL_VERTEX_SHADER;
		if (name == "fragment")
			return GL_FRAGMENT_SHADER;
		if (name == "geometry")
			return GL_GEOMETRY_SHADER;
		if (name == "control")
			return GL_TESS_CONTROL_SHADER;
		if (name == "evaluation")
			return GL_TESS_EVALUATION_SHADER;
		if (name == "compute")
			return GL_COMPUTE_SHADER;
		return 0;
	}

	bool stackIsOk(const std::vector<sint32> &stack)
	{
		for (sint32 it : stack)
			if (it != 1)
				return false;
		return true;
	}

	bool allowParsingHash()
	{
		if (defines.count("allowParsingHash") == 0)
			return false;
		return toBool(defines["allowParsingHash"]);
	}

	void parse(const String &filename)
	{
		Holder<File> file = newFile(filename, FileMode(true, false));
		uint32 lineNumber = 0;
		std::vector<sint32> stack;
		for (String line; file->readLine(line);)
		{
			const String originalLine = line;
			lineNumber++;
			try
			{
				line = trim(line);
				if (line.empty())
				{
					output("");
					continue;
				}
				if (line[0] == '$' || (allowParsingHash() && line[0] == '#'))
				{
					line = trim(subString(line, 1, m));
					String cmd = split(line);
					line = trim(line);
					if (cmd == "if")
					{
						if (line.empty())
							CAGE_THROW_ERROR(Exception, "'$if' missing parameters");
						stack.push_back(evalExpToBool(eval(line)) ? 1 : 0);
					}
					else if (cmd == "once")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(Exception, "'$once' cannot have parameters");
						String name = Stringizer() + filename + ":" + lineNumber;
						stack.push_back(onces.find(name) == onces.end() ? 1 : 0);
						onces.insert(name);
					}
					else if (cmd == "else")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(Exception, "'$else' cannot have parameters");
						if (stack.empty())
							CAGE_THROW_ERROR(Exception, "unexpected '$else'");
						sint32 v = stack.back();
						stack.pop_back();
						switch (v)
						{
						case 0: stack.push_back(1); break;
						case 1: stack.push_back(2); break;
						case 2: stack.push_back(2); break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid stack value");
						}
					}
					else if (cmd == "end" || cmd == "endif")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(Exception, "'$end' cannot have parameters");
						if (stack.empty())
							CAGE_THROW_ERROR(Exception, "unexpected '$end'");
						stack.pop_back();
					}
					else if (cmd == "stack")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(Exception, "'$stack' cannot have parameters");
						String s("// CAGE: stack:");
						for (const auto &it : stack)
							s += Stringizer() + " " + it;
						output(s);
					}
					else if (stackIsOk(stack))
					{
						if (cmd == "define" || cmd == "set")
						{
							String name = split(line);
							if (name.empty() || line.empty())
							{
								CAGE_LOG_THROW(Stringizer() + "name: '" + name + "'");
								CAGE_THROW_ERROR(Exception, "'$define/set' expects two parameters");
							}
							if (!validDefine(name))
							{
								CAGE_LOG_THROW(Stringizer() + "name: '" + name + "', value: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$define/set' with invalid name");
							}
							if (cmd == "set")
								line = eval(line);
							if (name == "shader" && shaderType(line) == 0)
								CAGE_THROW_ERROR(Exception, "'$define/set shader ...' must be one of vertex, fragment, geometry, control, evaluation or compute");
							defines[name] = line;
						}
						else if (cmd == "undef")
						{
							if (!validDefine(line))
							{
								CAGE_LOG_THROW(Stringizer() + "name: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$undef' with invalid name");
							}
							defines.erase(line);
						}
						else if (cmd == "print")
						{
							if (!validDefine(line))
							{
								CAGE_LOG_THROW(Stringizer() + "name: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$print' with invalid name");
							}
							if (defines.find(line) == defines.end())
							{
								CAGE_LOG_THROW(Stringizer() + "name: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$print' with unknown name");
							}
							output(defines[line]);
						}
						else if (cmd == "eval")
						{
							output(eval(line));
						}
						else if (cmd == "variables")
						{
							if (!line.empty())
								CAGE_THROW_ERROR(Exception, "'$variables' expects no parameters");
							for (const auto &it : defines)
								output(String() + "// CAGE: variable: '" + it.first + "' = '" + it.second + "'");
						}
						else if (cmd == "include")
						{
							if (line.empty())
								CAGE_THROW_ERROR(Exception, "'$include' expects one parameter");
							line = pathJoin(pathExtractDirectory(pathToRel(filename, inputDirectory)), line);
							writeLine(Stringizer() + "use=" + line);
							String fn = pathJoin(inputDirectory, line);
							if (!pathIsFile(fn))
							{
								CAGE_LOG_THROW(Stringizer() + "requested file '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$include' file not found");
							}
							if (configShaderPrint)
								output(Stringizer() + "// CAGE: include file: '" + line + "'");
							parse(fn);
							if (configShaderPrint)
								output(Stringizer() + "// CAGE: return to file: '" + pathToRel(filename, inputDirectory) + "':" + lineNumber);
						}
						else
						{
							CAGE_LOG_THROW(Stringizer() + "command: '" + cmd + "', params: '" + line + "'");
							CAGE_THROW_ERROR(Exception, "unknown '$' command");
						}
					}
				}
				else if (stackIsOk(stack))
					output(outputTokenization(originalLine));
			}
			catch (...)
			{
				CAGE_LOG_THROW(Stringizer() + "in file: '" + filename + "':" + lineNumber);
				CAGE_LOG_THROW(Stringizer() + "original line: '" + originalLine + "'");
				throw;
			}
		}

		if (!stack.empty())
		{
			CAGE_LOG_THROW(Stringizer() + "in file: '" + filename + "':" + lineNumber);
			CAGE_THROW_ERROR(Exception, "unexpected end of file; expecting '$end'");
		}
	}
}

void processShader()
{
	writeLine(String("use=") + inputFile);

	defines["cageShaderProcessor"] = "1";
	defines["inputSpec"] = inputSpec;

	parse(inputFileName);

	{
		String prepend;
		if (!properties("version").empty())
			prepend += String("#version ") + properties("version") + "\n";
		prepend += String() + "// " + inputName + "\n";
		for (auto &it : codes)
			it.second = std::string(prepend.c_str(), prepend.length()) + it.second;
	}

	{
		MemoryBuffer buff;
		Serializer ser(buff);
		ser << numeric_cast<uint32>(codes.size());
		for (const auto &it : codes)
		{
			ser << (uint32)shaderType(it.first);
			ser << numeric_cast<uint32>(it.second.length());
			ser.write({ it.second.c_str(), it.second.c_str() + it.second.length() });
			//ser.write(it.second);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "stage: " + it.first + ", length: " + it.second.size());
		}

		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (before compression): " + buff.size());
		Holder<PointerRange<char>> comp = compress(buff);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (after compression): " + comp.size());

		AssetHeader h = initializeAssetHeader();
		h.originalSize = buff.size();
		h.compressedSize = comp.size();
		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(comp);
		f->close();
	}

	if (configShaderPrint)
	{
		for (const auto &it : codes)
		{
			String name = pathJoin(configGetString("cage-asset-processor/shader/path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + "_" + it.first + ".glsl");
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(name, fm);
			f->write({ it.second.c_str(), it.second.c_str() + it.second.length() });
			f->close();
		}
	}
}
