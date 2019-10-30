namespace cage
{
	CAGE_API void checkGlError();

#ifdef CAGE_DEBUG
#define CAGE_CHECK_GL_ERROR_DEBUG() { try { checkGlError(); } catch (const ::cage::graphicsError &) { CAGE_LOG(severityEnum::Error, "exception", "opengl error cought in file '" __FILE__ "' at line " CAGE_STRINGIZE(__LINE__) ); } }
#else
#define CAGE_CHECK_GL_ERROR_DEBUG()
#endif

	struct CAGE_API graphicsError : public systemError
	{
		graphicsError(const char *file, uint32 line, const char *function, severityEnum severity, const char *message, uint32 code) noexcept;
	};

	namespace detail
	{
		CAGE_API void purgeGlShaderCache();
	}
}
