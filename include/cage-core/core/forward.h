namespace cage
{
	// core
	namespace detail
	{
		template<uint32 N> struct stringBase;
	}
	typedef detail::stringBase<1000> string;

	// math
	struct real;
	struct rads;
	struct degs;
	struct vec2;
	struct vec3;
	struct vec4;
	struct quat;
	struct mat3;
	struct mat4;
	struct transform;

	// geometry
	struct line;
	struct triangle;
	struct plane;
	struct sphere;
	struct aabb;

	// entities
	struct entityManagerCreateConfig;
	class componentClass;
	class entityClass;
	class groupClass;
	class entityManagerClass;

	// concurrent
	class mutexClass;
	class barrierClass;
	class semaphoreClass;
	class threadClass;

	// filesystem
	struct fileMode;
	class fileClass;
	class filesystemClass;
	class directoryListClass;
	class changeWatcherClass;

	// memory
	class virtualMemoryClass;

	// network
	class discoveryClientClass;
	class discoveryServerClass;
	class tcpConnectionClass;
	class tcpServerClass;
	class udpConnectionClass;
	class udpServerClass;

	// assets
	enum class assetStateEnum : uint32
	{
		Ready,
		NotFound,
		Error,
		Unknown,
	};
	struct assetManagerCreateConfig;
	struct assetContextStruct;
	struct assetSchemeStruct;
	class assetManagerClass;

	// utilities
	struct randomGenerator;
	struct lineReaderBuffer;
	struct memoryBuffer;
	struct collisionPairStruct;
	struct spatialDataCreateConfig;
	struct collisionDataCreateConfig;
	template<class T> class hashTableClass;
	class iniClass;
	class pngImageClass;
	class programClass;
	class threadPoolClass;
	template<class T> class threadSafeQueueClass;
	class threadSafeDoubleBufferControllerClass;
	class timerClass;
	class colliderClass;
	class spatialDataClass;
	class spatialQueryClass;
	class collisionDataClass;
	class collisionQueryClass;
	class textPackClass;
}
