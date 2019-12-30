#include "processor.h"

#include <cage-core/timer.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/hashString.h>
#include <cage-engine/opengl.h>

#include <map>
#include <set>
#include <vector>
#include <string>

namespace
{
	std::map<string, std::string> codes;
	std::map<string, string> defines;
	std::set<string, stringComparatorFast> onces;

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

	bool validDefine(const string &s)
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

	string outputTokenization(string input)
	{
		string result;
		while (!input.empty())
		{
			uint32 v = 0;
			for (; v < input.length(); v++)
				if (!validDefineChar(input[v]))
					break;
			if (v > 0)
			{
				string token = input.subString(0, v);
				input = input.remove(0, v);
				if (token.toUpper() == token)
				{
					if (defines.find(token) != defines.end())
						token = defines[token];
				}
				result += token;
			}
			else
			{
				result += string(&input[0], 1);
				input = input.remove(0, 1);
			}
		}
		return result;
	}

	bool evalExpToBool(const string &l)
	{
		if (l.isReal(true))
			return l.toFloat() != 0;
		else if (l.isBool())
			return l.toBool();
		else
		{
			CAGE_LOG(SeverityEnum::Note, "exception", string("expression: ") + l);
			CAGE_THROW_ERROR(Exception, "expression cannot be converted to bool");
		}
	}

	string eval(const string &input);

	string evalExp(const string &l)
	{
		if (l.empty())
			CAGE_THROW_ERROR(Exception, "unexpected end of line");
		uint32 p = l.find('|');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			return string(evalExpToBool(left) || evalExpToBool(right));
		}
		p = l.find('&');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			return string(evalExpToBool(left) && evalExpToBool(right));
		}
		p = l.find('<');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			if (left.isReal(true) && right.isReal(true))
				return string(left.toFloat() < right.toFloat());
			else
				return string(left < right);
		}
		p = l.find('>');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			if (left.isReal(true) && right.isReal(true))
				return string(left.toFloat() > right.toFloat());
			else
				return string(left > right);
		}
		p = l.find('=');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			if (left.isInteger(true) && right.isInteger(true))
				return string(left.toSint32() == right.toSint32());
			else
				return string(left == right);
		}
		p = l.find('-');
		if (p != m)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, m)).toSint32();
			return string(left - right);
		}
		p = l.find('+');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			if (left.isInteger(true) && right.isInteger(true))
				return string(left.toSint32() + right.toSint32());
			else
				return left + right;
		}
		p = l.find('%');
		if (p != m)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, m)).toSint32();
			return string(left % right);
		}
		p = l.find('/');
		if (p != m)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, m)).toSint32();
			return string(left / right);
		}
		p = l.find('*');
		if (p != m)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, m)).toSint32();
			return string(left * right);
		}
		p = l.find('^');
		if (p != m)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, m));
			if (right.isInteger(false))
			{
				uint32 index = right.toUint32();
				if (index < left.length())
					return string(&left[index], 1);
			}
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "expression: '" + l + "'");
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "string: '" + left + "'");
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "index: '" + right + "'");
				CAGE_THROW_ERROR(Exception, "non integer index or out of bounds");
			}
		}
		if (l[0] == '!')
		{
			return string(!evalExpToBool(evalExp(l.subString(1, m))));
		}
		if (defines.count(l))
		{
			return defines[l];
		}
		return l;
	}

	string eval(const string &input)
	{
		string l = input;
		l = l.replace(" ", "").replace("\t", "").replace("\n", "");
		while (true)
		{
			uint32 z = l.find(')');
			if (z == m)
				break;
			uint32 o = l.subString(0, z).reverse().find('(');
			if (o == m)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "expression: '" + input + "'");
				CAGE_THROW_ERROR(Exception, "unmatched ')'");
			}
			l = l.replace(z - o - 1, o + 2, evalExp(l.subString(z - o, o)));
		}
		if (l.find('(') != m)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "expression: '" + input + "'");
			CAGE_THROW_ERROR(Exception, "unmatched '('");
		}
		return evalExp(l);
	}

	void output(const string &s)
	{
		if (defines["shader"].empty())
		{
			if (!s.empty())
			{
				CAGE_LOG_DEBUG(SeverityEnum::Warning, logComponentName, stringizer() + "output to unspecified shader: '" + s + "'");
			}
			return;
		}
		codes[defines["shader"]] += std::string(s.c_str(), s.length()) + "\n";
	}

	uint32 shaderType(const string &name)
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
		return defines["allowParsingHash"].toBool();
	}

	void parse(const string &filename)
	{
		Holder<File> file = newFile(filename, FileMode(true, false));
		uint32 lineNumber = 0;
		std::vector<sint32> stack;
		for (string line; file->readLine(line);)
		{
			const string originalLine = line;
			lineNumber++;
			try
			{
				line = line.trim();
				if (line.empty())
				{
					output("");
					continue;
				}
				if (line[0] == '$' || (allowParsingHash() && line[0] == '#'))
				{
					line = line.subString(1, m).trim();
					string cmd = line.split();
					line = line.trim();
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
						string name = stringizer() + filename + ":" + lineNumber;
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
						string s("// CAGE: stack:");
						for (const auto &it : stack)
							s += stringizer() + " " + it;
						output(s);
					}
					else if (stackIsOk(stack))
					{
						if (cmd == "define" || cmd == "set")
						{
							string name = line.split();
							if (name.empty() || line.empty())
							{
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + name + "'");
								CAGE_THROW_ERROR(Exception, "'$define/set' expects two parameters");
							}
							if (!validDefine(name))
							{
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + name + "', value: '" + line + "'");
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
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$undef' with invalid name");
							}
							defines.erase(line);
						}
						else if (cmd == "print")
						{
							if (!validDefine(line))
							{
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$print' with invalid name");
							}
							if (defines.find(line) == defines.end())
							{
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + line + "'");
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
								output(string() + "// CAGE: variable: '" + it.first + "' = '" + it.second + "'");
						}
						else if (cmd == "include")
						{
							if (line.empty())
								CAGE_THROW_ERROR(Exception, "'$include' expects one parameter");
							line = pathJoin(pathExtractPath(pathToRel(filename, inputDirectory)), line);
							writeLine(stringizer() + "use=" + line);
							string fn = pathJoin(inputDirectory, line);
							if (!pathIsFile(fn))
							{
								CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "requested file '" + line + "'");
								CAGE_THROW_ERROR(Exception, "'$include' file not found");
							}
							if (configShaderPrint)
								output(stringizer() + "// CAGE: include file: '" + line + "'");
							parse(fn);
							if (configShaderPrint)
								output(stringizer() + "// CAGE: return to file: '" + pathToRel(filename, inputDirectory) + "':" + lineNumber);
						}
						else
						{
							CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "command: '" + cmd + "', params: '" + line + "'");
							CAGE_THROW_ERROR(Exception, "unknown '$' command");
						}
					}
				}
				else if (stackIsOk(stack))
					output(outputTokenization(originalLine));
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "in file: '" + filename + "':" + lineNumber);
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "original line: '" + originalLine + "'");
				throw;
			}
		}

		if (!stack.empty())
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "in file: '" + filename + "':" + lineNumber);
			CAGE_THROW_ERROR(Exception, "unexpected end of file; expecting '$end'");
		}
	}
}

void processShader()
{
	writeLine(string("use=") + inputFile);

	defines["cageShaderProcessor"] = "1";
	defines["inputSpec"] = inputSpec;

	parse(inputFileName);

	{
		string prepend;
		if (!properties("version").empty())
			prepend += string("#version ") + properties("version") + "\n";
		prepend += string() + "// " + inputName + "\n";
		uint32 y, M, d, h, m, s;
		detail::getSystemDateTime(y, M, d, h, m, s);
		prepend += string() + "// " + detail::formatDateTime(y, M, d, h, m, s) + "\n";
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
			ser.write(it.second.c_str(), it.second.length());
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "stage: " + it.first + ", length: " + it.second.size());
		}

		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
		MemoryBuffer comp = detail::compress(buff);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

		AssetHeader h = initializeAssetHeader();
		h.originalSize = numeric_cast<uint32>(buff.size());
		h.compressedSize = numeric_cast<uint32>(comp.size());
		Holder<File> f = newFile(outputFileName, FileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(comp.data(), comp.size());
		f->close();
	}

	if (configShaderPrint)
	{
		for (const auto &it : codes)
		{
			string name = pathJoin(configGetString("cage-asset-processor/shader/path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + "_" + it.first + ".glsl");
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(name, fm);
			f->write(it.second.c_str(), it.second.length());
		}
	}
}
