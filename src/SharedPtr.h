#ifndef PERYAN_SHARED_PTR_H__
#define PERYAN_SHARED_PTR_H__

namespace Peryan {

template <typename T>
class SharedPtr {
private:
	class SharedCount {
	private:
		class CountedBase {
		private:
			int useCount_;
		public:
			CountedBase() : useCount_(1) {}
			virtual ~CountedBase() {}

			virtual void dispose() = 0;

			void hold() {
				++useCount_;
				return;
			}

			void release() {
				--useCount_;
				if (useCount_ <= 0) {
					dispose();
					delete this;
				}
				return;
			}
		};

		template <typename U>
		class CountedImpl : public CountedBase {
		private:
			U *ptr_;
		public:
			CountedImpl(U *ptr) : ptr_(ptr) {}
			void dispose() {
				delete ptr_;
				return;
			}
		};

		CountedBase *impl_;

	public:
		SharedCount() : impl_(NULL) {}

		template <typename U> explicit SharedCount(U *ptr)
			: impl_(new CountedImpl<U>(ptr)) {}

		SharedCount(const SharedCount& sc) : impl_(sc.impl_) {
			if (impl_ != NULL)
				impl_->hold();
		}

		~SharedCount() {
			if (impl_ != NULL)
				impl_->release();
		}

		SharedCount& operator=(const SharedCount& sc) {
			CountedBase *prev = impl_;
			CountedBase *next = sc.impl_;

			if (prev != next) {
				if (next != NULL)
					next->hold();
				if (prev != NULL)
					prev->release();
				impl_ = next;
			}

			return *this;
		}
	};

	T *ptr_;
	SharedCount count_;

public:
	SharedPtr() : ptr_(NULL), count_() {}

	template <typename U> explicit SharedPtr(U *ptr)
		: ptr_(ptr), count_(ptr) {}

	template <typename U> SharedPtr(const SharedPtr<U>& sp)
		: ptr_(sp.ptr_), count_(sp.count_) {}

	T& operator*() const {
		return *ptr_;
	}

	T* operator->() const {
		return ptr_;
	}
};

}

#endif
