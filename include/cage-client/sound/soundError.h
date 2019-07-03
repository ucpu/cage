namespace cage
{
	struct soundError : public codeException
	{
		soundError(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept;
		virtual soundError &log();
	};
}
