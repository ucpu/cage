namespace cage
{
	CAGE_API void checkGlError();

#ifdef CAGE_DEBUG
#define CAGE_CHECK_GL_ERROR_DEBUG() { try { checkGlError(); } catch (const ::cage::GraphicsError &) { CAGE_LOG(SeverityEnum::Error, "exception", "opengl error cought in file '" __FILE__ "' at line " CAGE_STRINGIZE(__LINE__) ); } }
#else
#define CAGE_CHECK_GL_ERROR_DEBUG()
#endif

	struct CAGE_API GraphicsError : public SystemError
	{
		GraphicsError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
	};

	namespace detail
	{
		CAGE_API void purgeGlShaderCache();
	}
}
