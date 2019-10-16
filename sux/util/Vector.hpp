/*
 * Sux: Succinct data structures
 *
 * Copyright (C) 2019 Stefano Marchini
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../support/common.hpp"
#include <assert.h>
#include <iostream>
#include <string>
#include <sys/mman.h>

namespace sux::util {

/** Possible types of memory allocation.
 *
 * \see https://www.kernel.org/doc/html/latest/admin-guide/mm/hugetlbpage.html
 * \see https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html
 */
enum AllocType {
	/** Standard allocation with `malloc()` (usually, the default). */
	MALLOC,
	/** Allocation with `mmap()`. Allocations are aligned on a memory page (typically, 4KiB). */
	SMALLPAGE,
	/** Transparent huge pages support through `mmap()` and `madvise()`
	 * on Linux. Allocations are usually a mutiple of 4KiB, but they can be defragmented in blocks of 2MiB each. */
	TRANSHUGEPAGE,
	/** Direct huge page support through `mmap()` on Linux.
	 * In this case allocations are aligned on a huge (typically, 2MiB) memory page.
	 * This feature is usually disabled by default and it requires the administrator 
	 * to pre-reserve space for huge memory pages as documented in the reported external references  */
	FORCEHUGEPAGE
};

/** An expandable vector with settable type of memory allocation.
 *
 *  Instances of this class have a behavior similar to std::vector.
 *  However, the strategy used for allocation memory can be selected.
 *
 *  Once enough capacity has been allocated through reserve(size_t),
 *  p() is a pointer to the first element and all allocated space can
 *  be used directly, if necessary.
 *
 *  Vector implements the standard `<<` and `>>` operators for simple
 *  serialization and deserialization.
 *
 *  @tparam T the data type.
 *  @tparam AT a type of memory allocation out of ::AllocType.
 */

template <typename T, AllocType AT = MALLOC> class Vector {

  public:
	static constexpr int PROT = PROT_READ | PROT_WRITE;
	static constexpr int FLAGS = MAP_PRIVATE | MAP_ANONYMOUS | (AT == FORCEHUGEPAGE ? MAP_HUGETLB : 0);

  private:
	size_t _size = 0, _capacity = 0;
	T *data = nullptr;

  public:
	Vector<T, AT>() = default;

	explicit Vector<T, AT>(size_t size) { resize(size); }

	~Vector<T, AT>() {
		if (data) {
			if (AT == MALLOC) {
				free(data);
			} else {
				int result = munmap(data, _capacity);
				assert(result == 0 && "mmunmap failed");
			}
		}
	}

	Vector(Vector<T, AT> &&oth) : _size(std::exchange(oth._size, 0)), _capacity(std::exchange(oth._capacity, 0)), data(std::exchange(oth.data, nullptr)) {}

	Vector<T, AT> &operator=(Vector<T, AT> &&oth) {
		swap(*this, oth);
		return *this;
	}

	/** Trim the the memory allocated to the given size, if possible.
	 *
	 * @param size new desired size (in elements) of the allocated space.
	 */
	void trim(size_t size) {
		if (size < _capacity) remap(size);
	}

	/** Reserve enough space to contain a given number of elements.
	 *
	 * @param size how much space (in elements) to reserve.
	 *
	 * Nothing happens if the requested space is already reserved.
	 */
	void reserve(size_t size) {
		if (size > _capacity) remap(size);
	}

	/** Changes the vector size to the given value.
	  *
	  * If the argument is smaller than the current size, the excess
	  * elements will be discared. Otherwise, new zero elements will
	  * be added.
	  *
	  * @param size the desired new size (in elements) of this vector.
	  */
	void resize(size_t size) {
		// TODO: is this right?
		if (size > _capacity) reserve(max(size_t(4), (size * 3) / 2));
		_size = size;
	}

	/** Adds a given element at the end of this vector.
	 *
	 * @param elem an element.
	 */
	void pushBack(T elem) {
		resize(size + 1);
		data[size - 1] = elem;
	}

	/** Pops the element at the end of this vector.
	 *
	 *  The last element of this vector is removed and
     *  returned.
	 *
	 * @return the last element of this vector.
	 */
	T popBack() { return data[--_size]; }

	friend void swap(Vector<T, AT> &first, Vector<T, AT> &second) noexcept {
		std::swap(first._size, second._size);
		std::swap(first._capacity, second._capacity);
		std::swap(first.data, second.data);
	}

	/** Returns a pointer at the start of the backing array. */
	inline T *p() const { return data; }

	/** Returns the given element of the vector. */
	inline T &operator[](size_t i) const { return data[i]; };

	/** Returns the number of elements in this vector. */
	inline size_t size() const { return _size; }

	/** Returns the number of elements that this vector
	 * can hold currently without increasing its capacity.
	 *
	 * @return the number of elements that this vector
	 * can hold currently without increasing its capacity.
	 */
	inline size_t capacity() const { return _capacity; }

	/** Returns the number of bits used by this vector.
	 * @return the number of bits used by this vector.
	 */
	size_t bitCount() const { return sizeof(Vector<T, AT>) * 8 + _capacity * sizeof(T) * 8; }

  private:
	static size_t page_aligned(size_t size) {
		if (AT == FORCEHUGEPAGE)
			return ((2 * 1024 * 1024 - 1) | (size * sizeof(T) - 1)) + 1;
		else
			return ((4 * 1024 - 1) | (size * sizeof(T) - 1)) + 1;
	}

	void remap(size_t size) {
		if (size == 0) return;

		void *mem;
		size_t space; // Space to allocate, in bytes

		if (AT == MALLOC) {
			space = size * sizeof(T);
			mem = _capacity == 0 ? malloc(space) : realloc(data, space);
			assert(mem != NULL && "malloc failed");
		} else {
			space = page_aligned(size * sizeof(T));
			mem = _capacity == 0 ? mmap(nullptr, space, PROT, FLAGS, -1, 0) : mremap(data, _capacity * sizeof(T), space, MREMAP_MAYMOVE, -1, 0);
			assert(mem != MAP_FAILED && "mmap failed");

			if (AT == TRANSHUGEPAGE) {
				int adv = madvise(mem, space, MADV_HUGEPAGE);
				assert(adv == 0 && "madvise failed");
			}
		}

		if (_capacity * sizeof(T) < space) memset(static_cast<char *>(mem) + _capacity * sizeof(T), 0, space - _capacity * sizeof(T));

		_capacity = space / sizeof(T);
		data = static_cast<T *>(mem);
	}

	friend std::ostream &operator<<(std::ostream &os, const Vector<T, AT> &vector) {
		const uint64_t nsize = vector.size();
		os.write((char *)&nsize, sizeof(uint64_t));
		os.write((char *)vector.p(), vector.size() * sizeof(T));
		return os;
	}

	friend std::istream &operator>>(std::istream &is, Vector<T, AT> &vector) {
		uint64_t nsize;
		is.read((char *)&nsize, sizeof(uint64_t));
		vector = Vector<T, AT>(nsize);
		is.read((char *)vector.p(), vector.size() * sizeof(T));
		return is;
	}
};

} // namespace sux::util
