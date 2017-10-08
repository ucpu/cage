namespace cage
{
	struct soundException : public codeException
	{
		soundException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept;
		virtual soundException &log();
	};
}
