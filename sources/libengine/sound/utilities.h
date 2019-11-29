namespace cage
{
	namespace soundPrivat
	{
		void channelsMixing(float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 frames);
		void channelsDirection(float *bufferIn, float *bufferOut, uint32 channelsOut, uint32 frames, const vec3 &direction);
		void channelsOrdering(float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 frames, uint32 *channelsOrdering);
		void resample(float *bufferIn, float *bufferOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut, uint32 channels);
		void transfer(std::vector<float> &temporary, float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut);

		namespace convertSpace
		{
			template<class S, class T> struct convertStruct
			{};

			template<> struct convertStruct<float, double>
			{
				void operator ()(float source, double &target)
				{
					target = (double)source;
				}
			};

			template<> struct convertStruct<double, float>
			{
				void operator ()(double source, float &target)
				{
					target = (float)source;
				}
			};

			template<> struct convertStruct<float, float>
			{
				void operator ()(float source, float &target)
				{
					target = source;
				}
			};

			template<> struct convertStruct<double, double>
			{
				void operator ()(double source, double &target)
				{
					target = source;
				}
			};

			template<class T> struct convertStruct<T, float>
			{
				void operator ()(T source, float &target)
				{
					static_assert(detail::numeric_limits<T>::is_specialized, "invalid conversion type");
					static_assert(detail::numeric_limits<T>::is_signed, "invalid conversion type");
					static const float range = (float)detail::numeric_limits<T>::max() - (float)detail::numeric_limits<T>::min();
					target = (float)(source) / range * 2.f;
				}
			};

			template<class T> struct convertStruct<float, T>
			{
				void operator ()(float source, T &target)
				{
					static_assert(detail::numeric_limits<T>::is_specialized, "invalid conversion type");
					static_assert(detail::numeric_limits<T>::is_signed, "invalid conversion type");
					static const float range = (float)detail::numeric_limits<T>::max() - (float)detail::numeric_limits<T>::min();
					target = (T)(source * range * 0.5f);
				}
			};

			template<class T> struct convertStruct<T, double>
			{
				void operator ()(T source, double &target)
				{
					static_assert(detail::numeric_limits<T>::is_specialized, "invalid conversion type");
					static_assert(detail::numeric_limits<T>::is_signed, "invalid conversion type");
					static const double range = (double)detail::numeric_limits<T>::max() - (double)detail::numeric_limits<T>::min();
					target = (double)(source) / range * 2.0;
				}
			};

			template<class T> struct convertStruct<double, T>
			{
				void operator ()(double source, T &target)
				{
					static_assert(detail::numeric_limits<T>::is_specialized, "invalid conversion type");
					static_assert(detail::numeric_limits<T>::is_signed, "invalid conversion type");
					static const double range = (double)detail::numeric_limits<T>::max() - (double)detail::numeric_limits<T>::min();
					target = (T)(source * range * 0.5);
				}
			};
		}

		template<class S, class T> void convertSingle(S source, T &target)
		{
			convertSpace::convertStruct<S, T> c;
			c(source, target);
		}

		template<class S, class T> void convertSingle(S source, void *target)
		{
			convertSingle<S, T>(source, *(T*)target);
		}

		template<class S, class T> void convertArray(S *source, T *target, uint32 samples)
		{
			while (samples--)
				convertSingle<S, T>(*source++, *target++);
		}

		template<class S, class T> void convertArray(S *source, void *target, uint32 samples)
		{
			while (samples--)
				convertSingle<S, T>(*source++, *((T*)target)++);
		}
	}
}
