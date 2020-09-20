#pragma once
#include "TUniquePtr.h"

namespace LambdaEngine
{
	/*
	* Struct Counting references in TWeak- and TSharedPtr
	*/
	struct PtrControlBlock
	{
	public:
		typedef uint32 RefType;

		inline PtrControlBlock()
			: m_WeakReferences(0)
			, m_StrongReferences(0)
		{
		}

		FORCEINLINE RefType AddWeakRef() noexcept
		{
			return m_WeakReferences++;
		}

		FORCEINLINE RefType AddStrongRef() noexcept
		{
			return m_StrongReferences++;
		}

		FORCEINLINE RefType ReleaseWeakRef() noexcept
		{
			return m_WeakReferences--;
		}

		FORCEINLINE RefType ReleaseStrongRef() noexcept
		{
			return m_StrongReferences--;
		}

		FORCEINLINE RefType GetWeakReferences() const noexcept
		{
			return m_WeakReferences;
		}

		FORCEINLINE RefType GetStrongReferences() const noexcept
		{
			return m_StrongReferences;
		}

	private:
		RefType m_WeakReferences;
		RefType m_StrongReferences;
	};

	/*
	* TDelete
	*/
	template<typename T>
	struct TDelete
	{
		using TType = T;

		FORCEINLINE void operator()(TType* pPtr)
		{
			delete pPtr;
		}
	};

	template<typename T>
	struct TDelete<T[]>
	{
		using TType = TRemoveExtent<T>;

		FORCEINLINE void operator()(TType* pPtr)
		{
			delete[] pPtr;
		}
	};

	/*
	* Base class for TWeak- and TSharedPtr
	*/
	template<typename T, typename D>
	class TPtrBase
	{
	public:
		template<typename TOther, typename DOther>
		friend class TPtrBase;

		FORCEINLINE T* Get() const noexcept
		{
			return m_pPtr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_pPtr;
		}

		FORCEINLINE uint32 GetStrongReferences() const noexcept
		{
			return m_Counter ? m_Counter->GetStrongReferences() : 0;
		}

		FORCEINLINE uint32 GetWeakReferences() const noexcept
		{
			return m_Counter ? m_Counter->GetWeakReferences() : 0;
		}

		FORCEINLINE T* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE bool operator==(T* pPtr) const noexcept
		{
			return (m_pPtr == pPtr);
		}

		FORCEINLINE bool operator!=(T* pPtr) const noexcept
		{
			return (m_pPtr != pPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_pPtr != nullptr);
		}

	protected:
		FORCEINLINE TPtrBase() noexcept
			: m_pPtr(nullptr)
			, m_Counter(nullptr)
		{
			static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
			static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
		}

		FORCEINLINE void InternalAddStrongRef() noexcept
		{
			// If the object has a pPtr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->AddStrongRef();
			}
		}

		FORCEINLINE void InternalAddWeakRef() noexcept
		{
			// If the object has a pPtr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->AddWeakRef();
			}
		}

		FORCEINLINE void InternalReleaseStrongRef() noexcept
		{
			// If the object has a pPtr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->ReleaseStrongRef();

				// When releasing the last strong reference we can destroy the pointer and counter
				if (m_Counter->GetStrongReferences() <= 0)
				{
					if (m_Counter->GetWeakReferences() <= 0)
					{
						delete m_Counter;
					}
					
					m_Deleter(m_pPtr);
					InternalClear();
				}
			}
		}

		FORCEINLINE void InternalReleaseWeakRef() noexcept
		{
			// If the object has a pPtr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->ReleaseWeakRef();
				
				PtrControlBlock::RefType strongRefs = m_Counter->GetStrongReferences();
				PtrControlBlock::RefType weakRefs 	= m_Counter->GetWeakReferences();
				if (weakRefs <= 0 && strongRefs <= 0)
				{
					delete m_Counter;
				}
			}
		}

		FORCEINLINE void InternalSwap(TPtrBase& other) noexcept
		{
			T* pTempPtr = m_pPtr;
			PtrControlBlock* pTempBlock = m_Counter;

			m_pPtr		= other.m_pPtr;
			m_Counter	= other.m_Counter;

			other.m_pPtr	= pTempPtr;
			other.m_Counter	= pTempBlock;
		}

		FORCEINLINE void InternalMove(TPtrBase&& other) noexcept
		{
			m_pPtr		= other.m_pPtr;
			m_Counter	= other.m_Counter;

			other.m_pPtr	= nullptr;
			other.m_Counter	= nullptr;
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalMove(TPtrBase<TOther, DOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			m_pPtr		= static_cast<TOther*>(other.m_pPtr);
			m_Counter	= other.m_Counter;

			other.m_pPtr	= nullptr;
			other.m_Counter	= nullptr;
		}

		FORCEINLINE void InternalConstructStrong(T* pPtr)
		{
			m_pPtr		= pPtr;
			m_Counter	= new PtrControlBlock();
			InternalAddStrongRef();
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructStrong(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_pPtr		= static_cast<T*>(pPtr);
			m_Counter = new PtrControlBlock();
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructStrong(const TPtrBase& other)
		{
			m_pPtr		= other.m_pPtr;
			m_Counter	= other.m_Counter;
			InternalAddStrongRef();
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther, DOther>& other)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_pPtr		= static_cast<T*>(other.m_pPtr);
			m_Counter	= other.m_Counter;
			InternalAddStrongRef();
		}
		
		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther, DOther>& other, T* pPtr)
		{
			m_pPtr		= pPtr;
			m_Counter	= other.m_Counter;
			InternalAddStrongRef();
		}
		
		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructStrong(TPtrBase<TOther, DOther>&& other, T* pPtr)
		{
			m_pPtr		= pPtr;
			m_Counter	= other.m_Counter;
			other.m_pPtr	= nullptr;
			other.m_Counter	= nullptr;
		}

		FORCEINLINE void InternalConstructWeak(T* pPtr)
		{
			m_pPtr		= pPtr;
			m_Counter	= new PtrControlBlock();
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_pPtr	= static_cast<T*>(pPtr);
			m_Counter = new PtrControlBlock();
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalConstructWeak(const TPtrBase& other)
		{
			m_pPtr	= other.m_pPtr;
			m_Counter = other.m_Counter;
			InternalAddWeakRef();
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther, DOther>& other)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_pPtr		= static_cast<T*>(other.m_pPtr);
			m_Counter	= other.m_Counter;
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalDestructWeak()
		{
			InternalReleaseWeakRef();
			InternalClear();
		}

		FORCEINLINE void InternalDestructStrong()
		{
			InternalReleaseStrongRef();
			InternalClear();
		}

		FORCEINLINE void InternalClear() noexcept
		{
			m_pPtr	= nullptr;
			m_Counter = nullptr;
		}

	protected:
		T* m_pPtr;
		PtrControlBlock* m_Counter;
		D m_Deleter;
	};

	/*
	* Forward Declarations
	*/
	template<typename TOther>
	class TWeakPtr;

	/*
	* TSharedPtr - RefCounted Scalar Pointer
	*/
	template<typename T>
	class TSharedPtr : public TPtrBase<T, TDelete<T>>
	{
		using TBase = TPtrBase<T, TDelete<T>>;

	public:
		FORCEINLINE TSharedPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TSharedPtr(std::nullptr_t) noexcept
			: TBase()
		{
		}

		FORCEINLINE explicit TSharedPtr(T* pPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(pPtr);
		}

		FORCEINLINE TSharedPtr(const TSharedPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(other);
		}

		FORCEINLINE TSharedPtr(TSharedPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(::Move(other));
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalMove<TOther>(::Move(other));
		}
		
		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& other, T* pPtr) noexcept
			: TBase()
		{
			TBase::template InternalConstructStrong<TOther>(other, pPtr);
		}
		
		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& other, T* pPtr) noexcept
			: TBase()
		{
			TBase::template InternalConstructStrong<TOther>(::Move(other), pPtr);
		}

		template<typename TOther>
		FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TUniquePtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther, TDelete<T>>(other.Release());
		}

		FORCEINLINE ~TSharedPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructStrong();
		}

		FORCEINLINE void Swap(TSharedPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return (TBase::GetStrongReferences() == 1);
		}

		FORCEINLINE T* operator->() const noexcept
		{
			return TBase::Get();
		}
		
		FORCEINLINE T& operator*() const noexcept
		{
			VALIDATE(TBase::m_pPtr != nullptr);
			return *TBase::m_pPtr;
		}
		
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructStrong(other);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(TSharedPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther>& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructStrong<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalMove<TOther>(::Move(other));
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(T* pPtr) noexcept
		{
			if (this->pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructStrong(pPtr);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}

		FORCEINLINE bool operator==(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}
	};

	/*
	* TSharedPtr - RefCounted Pointer for array types
	*/
	template<typename T>
	class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
	{
		using TBase = TPtrBase<T, TDelete<T[]>>;

	public:
		FORCEINLINE TSharedPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TSharedPtr(std::nullptr_t) noexcept
			: TBase()
		{
		}

		FORCEINLINE explicit TSharedPtr(T* pPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(pPtr);
		}

		FORCEINLINE TSharedPtr(const TSharedPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(other);
		}

		FORCEINLINE TSharedPtr(TSharedPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(::Move(other));
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther[]>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(other);
		}
		
		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther[]>& other, T* pPtr) noexcept
			: TBase()
		{
			TBase::template InternalConstructStrong<TOther>(other, pPtr);
		}
		
		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther[]>&& other, T* pPtr) noexcept
			: TBase()
		{
			TBase::template InternalConstructStrong<TOther>(::Move(other), pPtr);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther[]>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalMove<TOther>(::Move(other));
		}

		template<typename TOther>
		FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther[]>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TUniquePtr<TOther[]>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(other.Release());
		}

		FORCEINLINE ~TSharedPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructStrong();
		}

		FORCEINLINE void Swap(TSharedPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return (TBase::GetStrongReferences() == 1);
		}

		FORCEINLINE T& operator[](uint32 index) noexcept
		{
			VALIDATE(TBase::m_pPtr != nullptr);
			return TBase::m_pPtr[index];
		}
		
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructStrong(other);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(TSharedPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther[]>& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructStrong<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther[]>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalMove<TOther>(::Move(other));
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(T* pPtr) noexcept
		{
			if (this->pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructStrong(pPtr);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}

		FORCEINLINE bool operator==(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}
	};

	/*
	* TWeakPtr - Weak Pointer for scalar types
	*/
	template<typename T>
	class TWeakPtr : public TPtrBase<T, TDelete<T>>
	{
		using TBase = TPtrBase<T, TDelete<T>>;

	public:
		FORCEINLINE TWeakPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TWeakPtr(const TSharedPtr<T>& other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TSharedPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(other);
		}

		FORCEINLINE TWeakPtr(const TWeakPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(other);
		}

		FORCEINLINE TWeakPtr(TWeakPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(::Move(other));
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TWeakPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(TWeakPtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalMove<TOther>(::Move(other));
		}

		FORCEINLINE ~TWeakPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructWeak();
		}

		FORCEINLINE void Swap(TWeakPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsExpired() const noexcept
		{
			return (TBase::GetStrongReferences() < 1);
		}

		FORCEINLINE TSharedPtr<T> MakeShared() noexcept
		{
			const TWeakPtr& thisPtr = *this;
			return ::Move(TSharedPtr<T>(thisPtr));
		}

		FORCEINLINE T* operator->() const noexcept
		{
			return TBase::Get();
		}
		
		FORCEINLINE T& operator*() const noexcept
		{
			VALIDATE(TBase::m_pPtr != nullptr);
			return *TBase::m_pPtr;
		}
		
		FORCEINLINE TWeakPtr& operator=(const TWeakPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructWeak(other);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(TWeakPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther>& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructWeak<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(T* pPtr) noexcept
		{
			if (TBase::m_pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructWeak(pPtr);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}

		FORCEINLINE bool operator==(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}
	};

	/*
	* TWeakPtr - Weak Pointer for scalar types
	*/
	template<typename T>
	class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
	{
		using TBase = TPtrBase<T, TDelete<T[]>>;

	public:
		FORCEINLINE TWeakPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TWeakPtr(const TSharedPtr<T>& other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TSharedPtr<TOther[]>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(other);
		}

		FORCEINLINE TWeakPtr(const TWeakPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(other);
		}

		FORCEINLINE TWeakPtr(TWeakPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(::Move(other));
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TWeakPtr<TOther[]>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(TWeakPtr<TOther[]>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalMove<TOther>(::Move(other));
		}

		FORCEINLINE ~TWeakPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructWeak();
		}

		FORCEINLINE void Swap(TWeakPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsExpired() const noexcept
		{
			return (TBase::GetStrongReferences() < 1);
		}

		FORCEINLINE TSharedPtr<T[]> MakeShared() noexcept
		{
			const TWeakPtr& thisPtr = *this;
			return ::Move(TSharedPtr<T[]>(thisPtr));
		}
		
		FORCEINLINE T& operator[](uint32 index) noexcept
		{
			VALIDATE(TBase::m_pPtr != nullptr);
			return TBase::m_pPtr[index];
		}

		FORCEINLINE TWeakPtr& operator=(const TWeakPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructWeak(other);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(TWeakPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther[]>& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructWeak<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther[]>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(::Move(other));
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(T* pPtr) noexcept
		{
			if (TBase::m_pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructWeak(pPtr);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}

		FORCEINLINE bool operator==(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.pPtr);
		}

		FORCEINLINE bool operator!=(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.pPtr);
		}
	};

	/*
	* Creates a new object together with a SharedPtr
	*/
	template<typename T, typename... TArgs>
	std::enable_if_t<!std::is_array_v<T>, TSharedPtr<T>> MakeShared(TArgs&&... Args) noexcept
	{
		T* pRefCountedPtr = new T(Forward<TArgs>(Args)...);
		return ::Move(TSharedPtr<T>(pRefCountedPtr));
	}

	template<typename T>
	std::enable_if_t<std::is_array_v<T>, TSharedPtr<T>> MakeShared(uint32 size) noexcept
	{
		using TType = TRemoveExtent<T>;

		TType* pRefCountedPtr = new TType[size];
		return ::Move(TSharedPtr<T>(pRefCountedPtr));
	}

	/*
	* Casting functions
	*/

	// static_cast
	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> StaticCast(const TSharedPtr<T1>& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = static_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(pPointer, pRawPointer));
	}

	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> StaticCast(TSharedPtr<T1>&& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = static_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(::Move(pPointer), pRawPointer));
	}

	// const_cast
	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ConstCast(const TSharedPtr<T1>& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = const_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(pPointer, pRawPointer));
	}

	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ConstCast(TSharedPtr<T1>&& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = const_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(::Move(pPointer), pRawPointer));
	}

	// reinterpret_cast
	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ReinterpretCast(const TSharedPtr<T1>& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = reinterpret_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(pPointer, pRawPointer));
	}

	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> ReinterpretCast(TSharedPtr<T1>&& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = reinterpret_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(::Move(pPointer), pRawPointer));
	}

	// dynamic_cast
	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> DynamicCast(const TSharedPtr<T1>& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = dynamic_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(pPointer, pRawPointer));
	}

	template<typename T0, typename T1>
	std::enable_if_t<std::is_array_v<T0> == std::is_array_v<T0>, TSharedPtr<T0>> DynamicCast(TSharedPtr<T1>&& pPointer)
	{
		using TType = TRemoveExtent<T0>;
		
		TType* pRawPointer = dynamic_cast<TType*>(pPointer.Get());
		return ::Move(TSharedPtr<T0>(::Move(pPointer), pRawPointer));
	}

}