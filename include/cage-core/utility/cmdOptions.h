#ifndef guard_cmdOptions_h_dc2df731_5f21_43ed_8f42_6b0f17048dce_
#define guard_cmdOptions_h_dc2df731_5f21_43ed_8f42_6b0f17048dce_

/*
	bool a = false;
	const char *b = "";
	bool c = false;
	holder<cmdOptionsClass> cmd = newCmdOptions(argc, args, "ab:c");
	while (cmd->next())
	{
		switch(cmd->option())
		{
		case 'a':
			a = true;
			break;
		case 'b':
			b = cmd->argument();
			break;
		case 'c':
			c = true;
			break;
		}
	}
*/

namespace cage
{
	class CAGE_API cmdOptionsClass
	{
	public:
		bool next();
		char option() const;
		const char *argument() const;
	};

	CAGE_API holder<cmdOptionsClass> newCmdOptions(uint32 argc, const char *args[], const char *options);
}

#endif // guard_cmdOptions_h_dc2df731_5f21_43ed_8f42_6b0f17048dce_
