#include <map>
#include <set>
#include <vector>
#include <string>

#include "processor.h"

#include <cage-core/timer.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-client/opengl.h>

namespace
{
	std::map<string, std::string> codes;
	std::map<string, string> defines;
	std::set<string, stringComparatorFast> onces;

	configBool configShaderPrint("cage-asset-processor.shader.preview");

	const bool validDefineChar(const char c)
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

	const bool validDefine(const string &s)
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

	const string outputTokenization(string input)
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

	const bool evalExpToBool(const string &l)
	{
		if (l.isReal(true))
			return l.toFloat() != 0;
		else if (l.isBool())
			return l.toBool();
		else
		{
			CAGE_LOG(severityEnum::Note, "exception", string("expression: ") + l);
			CAGE_THROW_ERROR(exception, "expression cannot be converted to bool");
		}
	}

	const string eval(const string &input);

	const string evalExp(const string &l)
	{
		if (l.empty())
			CAGE_THROW_ERROR(exception, "unexpected end of line");
		uint32 p = l.find('|');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			return evalExpToBool(left) || evalExpToBool(right);
		}
		p = l.find('&');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			return evalExpToBool(left) && evalExpToBool(right);
		}
		p = l.find('<');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			if (left.isReal(true) && right.isReal(true))
				return left.toFloat() < right.toFloat();
			else
				return left < right;
		}
		p = l.find('>');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			if (left.isReal(true) && right.isReal(true))
				return left.toFloat() > right.toFloat();
			else
				return left > right;
		}
		p = l.find('=');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			if (left.isInteger(true) && right.isInteger(true))
				return left.toSint32() == right.toSint32();
			else
				return left == right;
		}
		p = l.find('-');
		if (p != -1)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, -1)).toSint32();
			return string(left - right);
		}
		p = l.find('+');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			if (left.isInteger(true) && right.isInteger(true))
				return string(left.toSint32() + right.toSint32());
			else
				return left + right;
		}
		p = l.find('%');
		if (p != -1)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, -1)).toSint32();
			return string(left % right);
		}
		p = l.find('/');
		if (p != -1)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, -1)).toSint32();
			return string(left / right);
		}
		p = l.find('*');
		if (p != -1)
		{
			sint32 left = evalExp(l.subString(0, p)).toSint32();
			sint32 right = evalExp(l.subString(p + 1, -1)).toSint32();
			return string(left * right);
		}
		p = l.find('^');
		if (p != -1)
		{
			string left = evalExp(l.subString(0, p));
			string right = evalExp(l.subString(p + 1, -1));
			if (right.isInteger(false))
			{
				uint32 index = right.toUint32();
				if (index >= left.length())
				{
					CAGE_LOG(severityEnum::Note, logComponentName, string("expression: ") + l + "'");
					CAGE_LOG(severityEnum::Note, logComponentName, string("string: ") + left + "'");
					CAGE_LOG(severityEnum::Note, logComponentName, string("index: ") + index + "'");
					CAGE_LOG(severityEnum::Warning, logComponentName, "substring index out of range");
					return "";
				}
				return string(&left[index], 1);
			}
			else
			{
				CAGE_LOG(severityEnum::Note, "exception", string("expression: '") + l + "'");
				CAGE_LOG(severityEnum::Note, "exception", string("string: '") + left + "'");
				CAGE_LOG(severityEnum::Note, "exception", string("index: '") + right + "'");
				CAGE_THROW_ERROR(exception, "non integer index");
			}
		}
		if (l[0] == '!')
		{
			return !evalExpToBool(evalExp(l.subString(1, -1)));
		}
		if (defines.find(l) != defines.end())
		{
			return eval(defines[l]);
		}
		return l;
	}

	const string eval(const string &input)
	{
		string l = input;
		l = l.replace(" ", "").replace("\t", "").replace("\n", "");
		while (true)
		{
			uint32 z = l.find(')');
			if (z == -1)
				break;
			uint32 o = l.subString(0, z).reverse().find('(');
			if (o == -1)
			{
				CAGE_LOG(severityEnum::Note, "exception", string("expression: '") + input + "'");
				CAGE_THROW_ERROR(exception, "unmatched ')'");
			}
			l = l.replace(z - o - 1, o + 2, evalExp(l.subString(z - o, o)));
		}
		if (l.find('(') != -1)
		{
			CAGE_LOG(severityEnum::Note, "exception", string("expression: '") + input + "'");
			CAGE_THROW_ERROR(exception, "unmatched '('");
		}
		return evalExp(l);
	}

	void output(const string &s)
	{
		if (s.empty() || defines["shader"].empty())
			return;
		codes[defines["shader"]] += std::string(s.c_str(), s.length()) + "\n";
	}

	const uint32 shaderType(const string &name)
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

	const bool stackIsOk(const std::vector <sint32> &stack)
	{
		for (std::vector<sint32>::const_iterator it = stack.begin(), et = stack.end(); it != et; it++)
			if (*it != 1)
				return false;
		return true;
	}

	void parse(const string &filename, const bool allowParsingHash = false)
	{
		holder<fileClass> file = newFile(filename, fileMode(true, false, true));
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
					continue;
				if (line[0] == '$' || (allowParsingHash && line[0] == '#'))
				{
					line = line.subString(1, -1).trim();
					string cmd = line.split();
					line = line.trim();
					if (cmd == "if")
					{
						if (line.empty())
							CAGE_THROW_ERROR(exception, "'$if' missing parameters");
						stack.push_back(evalExpToBool(eval(line)) ? 1 : 0);
					}
					else if (cmd == "once")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(exception, "'$once' cannot have parameters");
						string name = filename + ":" + lineNumber;
						stack.push_back(onces.find(name) == onces.end() ? 1 : 0);
						onces.insert(name);
					}
					else if (cmd == "else")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(exception, "'$else' cannot have parameters");
						if (stack.empty())
							CAGE_THROW_ERROR(exception, "unexpected '$else'");
						sint32 v = stack.back();
						stack.pop_back();
						switch (v)
						{
						case 0: stack.push_back(1); break;
						case 1: stack.push_back(2); break;
						case 2: stack.push_back(2); break;
						default: CAGE_THROW_CRITICAL(exception, "invalid stack value");
						}
					}
					else if (cmd == "end" || cmd == "endif")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(exception, "'$end' cannot have parameters");
						if (stack.empty())
							CAGE_THROW_ERROR(exception, "unexpected '$end'");
						stack.pop_back();
					}
					else if (cmd == "stack")
					{
						if (!line.empty())
							CAGE_THROW_ERROR(exception, "'$stack' cannot have parameters");
						string s("// CAGE: stack:");
						for (auto it = stack.begin(), et = stack.end(); it != et; it++)
							s += string(" ") + *it;
						output(s);
					}
					else if (stackIsOk(stack))
					{
						if (cmd == "define" || cmd == "set")
						{
							string name = line.split();
							if (name.empty() || line.empty())
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + name + "'");
								CAGE_THROW_ERROR(exception, "'$define/set' expects two parameters");
							}
							if (!validDefine(name))
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + name + "', value: '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$define/set' with invalid name");
							}
							if (cmd == "set")
								line = eval(line);
							if (name == "shader" && shaderType(line) == 0)
								CAGE_THROW_ERROR(exception, "'$define/set shader ...' must be one of vertex, fragment, geometry, control, evaluation or compute");
							defines[name] = line;
						}
						else if (cmd == "undef")
						{
							if (!validDefine(line))
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$undef' with invalid name");
							}
							defines.erase(line);
						}
						else if (cmd == "print")
						{
							if (!validDefine(line))
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$print' with invalid name");
							}
							if (defines.find(line) == defines.end())
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$print' with unknown name");
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
								CAGE_THROW_ERROR(exception, "'$variables' expects no parameters");
							for (auto it = defines.begin(), et = defines.end(); it != et; it++)
								output(string() + "// CAGE: variable: '" + it->first + "' = '" + it->second + "'");
						}
						else if (cmd == "include")
						{
							if (line.empty())
								CAGE_THROW_ERROR(exception, "'$include' expects one parameter");
							line = pathJoin(pathExtractPath(pathToRel(filename, inputDirectory)), line);
							writeLine(string("use=") + line);
							string fn = pathJoin(inputDirectory, line);
							if (!pathExists(fn))
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "requested file '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$include' file not found");
							}
							if (configShaderPrint)
								output(string() + "// CAGE: include file: '" + line + "'");
							parse(fn);
							if (configShaderPrint)
								output(string() + "// CAGE: return to file: '" + pathToRel(filename, inputDirectory) + "':" + lineNumber);
						}
						else if (cmd == "import")
						{
							if (line.empty())
								CAGE_THROW_ERROR(exception, "'$import' expects one parameter");
							if (!pathIsAbs(line))
								line = pathJoin(inputDirectory, line);
							if (!pathExists(line))
							{
								CAGE_LOG(severityEnum::Note, "exception", string() + "requested file '" + line + "'");
								CAGE_THROW_ERROR(exception, "'$import' file not found");
							}
							if (configShaderPrint)
								output(string() + "// CAGE: import file: '" + pathToRel(line, inputDirectory) + "'");
							parse(line, true);
							if (configShaderPrint)
								output(string() + "// CAGE: return to file: '" + pathToRel(filename, inputDirectory) + "':" + lineNumber);
						}
						else
						{
							CAGE_LOG(severityEnum::Note, "exception", string() + "command: '" + cmd + "', params: '" + line + "'");
							CAGE_THROW_ERROR(exception, "unknown '$' command");
						}
					}
				}
				else if (stackIsOk(stack))
					output(outputTokenization(originalLine));
			}
			catch (...)
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "in file: '" + filename + "':" + lineNumber);
				CAGE_LOG(severityEnum::Note, "exception", string() + "original line: '" + originalLine + "'");
				throw;
			}
		}

		if (!stack.empty())
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "in file: '" + filename + "':" + lineNumber);
			CAGE_THROW_ERROR(exception, "unexpected end of file; expecting '$end'");
		}
	}
}

void processShader()
{
	writeLine(string("use=") + inputFile);

	defines["cage_shader_processor"] = "1";
#define GCHL_GENERATE(S) defines[CAGE_STRINGIZE(S)] = S;
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, inputDirectory, inputName, outputDirectory, outputName, assetPath, schemePath, inputSpec));
#undef GCHL_GENERATE

	parse(inputFileName);

	{
		string prepend;
		if (!properties("version").empty())
			prepend += string("#version ") + properties("version") + "\n";
		prepend += string() + "// " + inputName + "\n";
		uint32 y, M, d, h, m, s;
		detail::getSystemDateTime(y, M, d, h, m, s);
		prepend += string() + "// " + detail::formatDateTime(y, M, d, h, m, s) + "\n";
		for (std::map <string, std::string>::iterator it = codes.begin(), et = codes.end(); it != et; it++)
			codes[it->first] = std::string(prepend.c_str(), prepend.length()) + it->second;
	}

	{
		memoryBuffer buff;
		serializer ser(buff);
		ser << numeric_cast<uint32>(codes.size());
		for (const auto &it : codes)
		{
			ser << (uint32)shaderType(it.first);
			ser << numeric_cast<uint32>(it.second.length());
			ser.write(it.second.c_str(), it.second.length());
		}

		CAGE_LOG(severityEnum::Info, logComponentName, string() + "buffer size (before compression): " + buff.size());
		memoryBuffer comp = detail::compress(buff);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "buffer size (after compression): " + comp.size());

		assetHeaderStruct h = initializeAssetHeaderStruct();
		h.originalSize = numeric_cast<uint32>(buff.size());
		h.compressedSize = numeric_cast<uint32>(comp.size());
		holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(comp.data(), comp.size());
		f->close();
	}

	if (configShaderPrint)
	{
		for (const auto &it : codes)
		{
			string name = pathJoin(configGetString("cage-asset-processor.shader.path", "asset-preview"), pathMakeValid(inputName) + "_" + it.first + ".glsl");
			holder<fileClass> f = newFile(name, fileMode(false, true, true));
			f->write(it.second.c_str(), it.second.length());
		}
	}
}
