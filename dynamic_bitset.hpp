#pragma once

#include <vector>
#include <iostream>
#include <climits>
#include <algorithm>
#include <cctype>


namespace bit
{


template <typename Chunk_T = unsigned long long, typename Allocator_T = std::allocator <Chunk_T>>
class Bitset
{
private:

	std::size_t size_;																			//size of the bitset 
	std::vector <Chunk_T> chunks_;																//array containing the bitset
	constexpr static std::size_t chunk_size_ = Bitset <Chunk_T, Allocator_T>::sizeof_chunk_();	//how many bits can a chunk hold

	//so we dont compute these a billion times, at runtime
	constexpr static Chunk_T set_chunk_ = ~Chunk_T(0); 											//1111 1111
	constexpr static Chunk_T one_chunk_ = Chunk_T(1);											//0000 0001
	constexpr static Chunk_T zero_chunk_ = Chunk_T(0);											//0000 0000

	//single bit abstraction
	class Reference
	{
		Chunk_T &chunk_reference_;

		const Chunk_T chunk_mask_true_;
		const Chunk_T chunk_mask_false_;

	public:

		constexpr explicit Reference(Bitset <Chunk_T, Allocator_T>&, const std::size_t);

		~Reference() noexcept = default;


		constexpr operator bool () const noexcept;


		constexpr Reference& operator = (const bool) noexcept;
		constexpr Reference& operator = (const Reference&) noexcept;


		constexpr Reference& operator &= (const bool) noexcept;
		constexpr Reference& operator |= (const bool) noexcept;
		constexpr Reference& operator ^= (const bool) noexcept;
		constexpr bool operator~() const noexcept;


	};

public:


	class Iterator
	{

	public:

		constexpr explicit Iterator();

		constexpr explicit Iterator(const Iterator&);

		~Iterator() noexcept = default;


		constexpr Iterator& operator ++ ();
		constexpr Iterator operator ++ (std::size_t);
		constexpr Iterator& operator -- ();
		constexpr Iterator operator -- (std::size_t);

		constexpr Iterator& operator += (std::size_t);
		constexpr Iterator& operator -= (std::size_t);

		constexpr Iterator operator + (std::size_t) const;
		//constexpr friend Iterator operator + (std::size_t, const Iterator&);

		constexpr Iterator operator - (std::size_t) const;
		//constexpr friend Iterator operator - (std::size_t, const Iterator&);

	};

public:

	constexpr explicit Bitset(const std::size_t = 0, const bool value = false); 				//default constructor							std::vector::vector() (3)

	constexpr explicit Bitset(const std::initializer_list <bool>);								//initializer list constructor					std::vector::vector() (9)

	~Bitset() noexcept = default;																//default destructor							std::vector::~vector()


//-------------------------ELEMENT ACCESS-------------------------

	constexpr bool test(const std::size_t) const;

	constexpr Reference at(const std::size_t);

	constexpr bool none() const;//


//-------------------------CAPACITY-------------------------

	constexpr std::size_t size() const noexcept;


//-------------------------MODIFIERS-------------------------

	constexpr Bitset <Chunk_T, Allocator_T>& clear() noexcept;

	constexpr Bitset <Chunk_T, Allocator_T>& resize(const std::size_t = 0, const bool = false);


	constexpr Bitset <Chunk_T, Allocator_T>& set();
	constexpr Bitset <Chunk_T, Allocator_T>& set(const std::size_t, const bool = true);

	constexpr Bitset <Chunk_T, Allocator_T>& reset();
	constexpr Bitset <Chunk_T, Allocator_T>& reset(const std::size_t);

	constexpr Bitset <Chunk_T, Allocator_T>& flip();
	constexpr Bitset <Chunk_T, Allocator_T>& flip(const std::size_t);

	constexpr Bitset <Chunk_T, Allocator_T>& operator &= (Bitset <Chunk_T, Allocator_T>&);
	constexpr Bitset <Chunk_T, Allocator_T>& operator |= (Bitset <Chunk_T, Allocator_T>&);
	constexpr Bitset <Chunk_T, Allocator_T>& operator ^= (Bitset <Chunk_T, Allocator_T>&);
	constexpr Bitset <Chunk_T, Allocator_T> operator ~ () const;

	constexpr Bitset <Chunk_T, Allocator_T> operator << (const std::size_t) const;
	constexpr Bitset <Chunk_T, Allocator_T>& operator <<= (const std::size_t);
	constexpr Bitset <Chunk_T, Allocator_T> operator >> (const std::size_t);					//cant be const since bit sanitization must be dealt with
	constexpr Bitset <Chunk_T, Allocator_T>& operator >>= (const std::size_t);


//-------------------------CONVERSIONS-------------------------

	template <typename Char_T = char, typename Traits_T = std::char_traits <Char_T>, typename Str_Alloc_T = std::allocator <Char_T>>
	constexpr std::basic_string <Char_T, Traits_T, Str_Alloc_T> to_string(const Char_T = Char_T('0'), const Char_T = Char_T('1')) const;


//-------------------------NON MEMBER FUNCTIOS-------------------------

	template <typename Chunk_FT, typename Allocator_FT, typename Char_T, typename Traits_T>
	friend std::basic_ostream <Char_T, Traits_T>& operator << (std::basic_ostream <Char_T, Traits_T>&, const Bitset <Chunk_FT, Allocator_FT>&);

private:

	//-------------------------helpers/getters-------------------------

	constexpr std::size_t needed_chunks_(const std::size_t size) const noexcept; 				//just a ceil function

	constexpr static std::size_t sizeof_chunk_() noexcept;										//chunk_size_ compile time calculator

	constexpr std::string oor_msg_builder_(const std::string&, const std::size_t) const;		//out of range which/pos string formatter

	constexpr std::size_t chunk_pos_(const std::size_t) const noexcept;

	constexpr std::size_t bit_pos_(const std::size_t) const noexcept;

	void set_unused_bits_(const bool = false);													//sanitization tool
	
};



/*
============================================================================
-------------------------class Bitset methods BEGIN-------------------------
============================================================================
*/

//-------------------------constructors-------------------------

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>::Bitset(const std::size_t size, const bool value)
	:
	size_(size),
	chunks_(needed_chunks_(size_), value ? set_chunk_ : zero_chunk_, Allocator_T())
{};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>::Bitset(const std::initializer_list <bool> init)
	:
	size_(init.size()),
	chunks_(Allocator_T())
{
	//optimized as to avoid checks and unneeded operations as much as possible (does anyone even use initializer list for big initializations lol?)
	chunks_.reserve(needed_chunks_(size_));

	Chunk_T temp_chunk = zero_chunk_;
	auto curr_bit = init.begin();

	const std::size_t full_chunks = size_ / chunk_size_;		//amount of bits that will be initialized as full chunks
	for(int itr = 0; itr < full_chunks; ++itr)
	{
		for(std::size_t bit_pos = 0; bit_pos < chunk_size_; ++bit_pos)
		{
			if(*(curr_bit++)) //check then increment
				temp_chunk |= (one_chunk_ << bit_pos);
		}

		chunks_.push_back(temp_chunk);
		temp_chunk = zero_chunk_;
	}

	const std::size_t remaining_bits = size_ % chunk_size_;		//the remaining (less than chunk_size_) bits that will be initialized as a half chunk
	for(std::size_t bit_pos = 0; bit_pos < remaining_bits; ++bit_pos)
	{
		if(*(curr_bit++))
			temp_chunk |= (one_chunk_ << bit_pos);
	}

	chunks_.push_back(temp_chunk);
};


//-------------------------ELEMENT ACCESS-------------------------

template <typename Chunk_T, typename Allocator_T>
constexpr bool Bitset <Chunk_T, Allocator_T>::test(const std::size_t pos) const
{
	if(pos >= size_)
		throw std::out_of_range(oor_msg_builder_("test", pos));

	return chunks_[chunk_pos_(pos)] & (one_chunk_ << bit_pos_(pos));
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference Bitset <Chunk_T, Allocator_T>::at(const std::size_t pos)
{
	if(pos >= size_)
		throw std::out_of_range(oor_msg_builder_("at", pos));

	return Reference(*this, pos);
};

template <typename Chunk_T, typename Allocator_T>
constexpr bool Bitset <Chunk_T, Allocator_T>::none() const
{

	std::cout << chunks_.size();

	for(std::size_t itr = 0; itr < chunks_.size() - 1; ++itr)
	{
		if(chunks_[itr] != zero_chunk_)
		{
			return false;
		}
	}

	const std::size_t remaining_bits = size_ % chunk_size_;		//the remaining (less than chunk_size_) bits that will be initialized as a half chunk

	if(remaining_bits)
	{
		if(chunks_.back() << (chunk_size_ - remaining_bits) != zero_chunk_)
		{
			return false;
		}
	}
	else
	{
		if(chunks_.back() != zero_chunk_)
		{
			return false;
		}
	}

	return true;

};


//-------------------------CAPACITY-------------------------

template <typename Chunk_T, typename Allocator_T> //pure
constexpr std::size_t Bitset <Chunk_T, Allocator_T>::size() const noexcept
{
	return size_;
};


//-------------------------MODIFIERS-------------------------

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::clear() noexcept
{
	size_ = 0;
	chunks_.clear(); //memory will still be allocated, but objects will be destroyed

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::resize(const std::size_t size, const bool value)
{
	set_unused_bits_(value); //set the newly added bits inside the last half chunk before doing any changes

	size_ = size;
	chunks_.resize(needed_chunks_(size), value ? set_chunk_ : zero_chunk_);

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::set()
{
	std::fill(chunks_.begin(), chunks_.end(), set_chunk_);

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::set(const std::size_t pos, const bool value)
{
	if(pos >= size_)
		throw std::out_of_range(oor_msg_builder_("set", pos));

	if(value)
		chunks_[chunk_pos_(pos)] |= (one_chunk_ << bit_pos_(pos));
	else
		chunks_[chunk_pos_(pos)] &= ~(one_chunk_ << bit_pos_(pos));

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::reset()
{
	std::fill(chunks_.begin(), chunks_.end(), zero_chunk_);

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::reset(const std::size_t pos)
{
	set(pos, false);

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::flip()
{
	for(auto &itr : chunks_)
		itr = ~itr;

	return *this;
};

template <typename Chunk_T, typename Allocator_T> 
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::flip(const std::size_t pos)
{
	if(pos >= size_)
		throw std::out_of_range(oor_msg_builder_("flip", pos));

	chunks_[chunk_pos_(pos)] ^= (one_chunk_ << bit_pos_(pos));

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::operator &= (Bitset <Chunk_T, Allocator_T> &rhs)
{
	std::size_t min_size = size_;

	if(min_size > rhs.size_)
	{
		rhs.set_unused_bits_(true); //set the unused bits to 1 so that &op wont mess around with the result (only needed if rhs chunk overlaps with this and its got unused bits)
		min_size = rhs.size_;
	}

	for(std::size_t itr = 0; itr < min_size; ++itr)
		chunks_[itr] &= rhs.chunks_[itr];

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::operator |= (Bitset <Chunk_T, Allocator_T> &rhs)
{
	std::size_t min_size = chunks_.size();

	if(min_size > rhs.chunks_.size())
	{
		rhs.set_unused_bits_(false); //set the unused bits to 0 so that &op wont mess around with the result (only needed if rhs chunk overlaps with this and its got unused bits)
		min_size = rhs.chunks_.size();
	}

	for(std::size_t itr = 0; itr < min_size; ++itr)
		chunks_[itr] |= rhs.chunks_[itr];

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::operator ^= (Bitset <Chunk_T, Allocator_T> &rhs)
{
	std::size_t min_size = size_;

	if(min_size > rhs.size_)
	{
		rhs.set_unused_bits_(false); //set the unused bits to 0 so that &op wont mess around with the result (only needed if rhs chunk overlaps with this and its got unused bits)
		min_size = rhs.size_;
	}

	for(std::size_t itr = 0; itr < min_size; ++itr)
		chunks_[itr] ^= rhs.chunks_[itr];

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T> Bitset <Chunk_T, Allocator_T>::operator ~ () const
{
	Bitset <Chunk_T, Allocator_T> flipped;
	flipped.chunks_.reserve(chunks_.size());
	flipped.size_ = size_;

	for(auto &chunk : chunks_)
		flipped.chunks_.push_back(~chunk);

	return flipped;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T> Bitset <Chunk_T, Allocator_T>::operator << (const std::size_t pos) const
{
	//shift chunks to right (increase value)
	//unused bits should still be outside reach, therefore no need to mess with them
	if(!pos)
		return *this;

	if(pos >= size_)
		return Bitset <Chunk_T, Allocator_T>(size_);

	Bitset <Chunk_T, Allocator_T> shifted(size_); //empty bitset

	const std::size_t left_offset = pos % chunk_size_;				//left part (smaller) of the chunk will become the right (larger) part of the new chunk
	const std::size_t right_offset = chunk_size_ - left_offset; 	//right part (larger) of the chunk will become the left (smallest) part of the new chunk
	const std::size_t chunk_offset = pos / chunk_size_;				//first chunk to be set in the new bitset

	if(pos % chunk_size_)
	{
		//we will set every chunk AFTER chunk_offset, we will deal with chunk_offset separately as it will only need part of it set
		for(std::size_t itr = chunks_.size() - 1; itr > chunk_offset; --itr)
			shifted.chunks_[itr] = (chunks_[itr - chunk_offset] << left_offset) | (chunks_[itr - chunk_offset - 1] >> right_offset);

		shifted.chunks_[chunk_offset] = chunks_[0] << left_offset;
	}
	else
	{
		//full chunk shift
		for(std::size_t itr = chunks_.size() - 1; itr >= chunk_offset; --itr)
			shifted.chunks_[itr] = chunks_[itr - chunk_offset];
	}

	return shifted;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::operator <<= (const std::size_t pos)
{
	if(!pos)
		return *this;

	if(pos >= size_)
	{
		std::fill(chunks_.begin(), chunks_.end(), zero_chunk_);
		return *this;
	}

	const std::size_t left_offset = pos % chunk_size_; 				//left part (smaller) of the chunk will become the right (larger) part of the new chunk
	const std::size_t right_offset = chunk_size_ - left_offset; 	//right part (larger) of the chunk will become the left (smallest) part of the new chunk
	const std::size_t chunk_offset = pos / chunk_size_;				//first chunk to be set in the new bitset

	if(pos % chunk_size_)
	{
		//we will set every chunk AFTER chunk_offset, we will deal with chunk_offset separately as it will only need part of it set
		for(std::size_t itr = chunks_.size() - 1; itr > chunk_offset; --itr)
			chunks_[itr] = (chunks_[itr - chunk_offset] << left_offset) | (chunks_[itr - chunk_offset - 1] >> right_offset);

		chunks_[chunk_offset] = chunks_[0] << left_offset;
	}
	else
	{
		//full chunk shift
		for(std::size_t itr = chunks_.size() - 1; itr >= chunk_offset; --itr)
			chunks_[itr] = chunks_[itr - chunk_offset];
	}

	std::fill(chunks_.begin(), chunks_.begin() + chunk_offset, zero_chunk_);

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T> Bitset <Chunk_T, Allocator_T>::operator >> (const std::size_t pos) //not const since might need to sanitize some bits...
{
	//shift chunks to left (decrease value)
	//unused bits are definetely a problem...
	if(!pos)
		return *this;

	if(pos >= size_)
		return Bitset <Chunk_T, Allocator_T>(size_);

	set_unused_bits_(false);

	Bitset <Chunk_T, Allocator_T> shifted(size_); //empty bitset

	const std::size_t left_offset = pos % chunk_size_; 				//left part (smaller) of the chunk will become the right (larger) part of the new chunk
	const std::size_t right_offset = chunk_size_ - left_offset; 	//right part (larger) of the chunk will become the left (smallest) part of the new chunk
	const std::size_t chunk_offset = pos / chunk_size_;				//last chunk to be set in the new bitset

	if(pos % chunk_size_)
	{
		//we will set every chunk AFTER chunk_offset, we will deal with chunk_offset separately as it will only need part of it set
		for(std::size_t itr = 0; itr < chunks_.size() - chunk_offset - 1; ++itr)
			shifted.chunks_[itr] = (chunks_[itr + chunk_offset] >> left_offset) | (chunks_[itr + chunk_offset + 1] << right_offset);

		shifted.chunks_[chunks_.size() - chunk_offset - 1] = chunks_[chunks_.size() - 1] >> left_offset;
	}
	else
	{
		//full chunk shift
		for(std::size_t itr = 0; itr <= chunks_.size() - chunk_offset - 1; ++itr)
			shifted.chunks_[itr] = chunks_[itr + chunk_offset];
	}

	return shifted;
};

template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>& Bitset <Chunk_T, Allocator_T>::operator >>= (const std::size_t pos)
{
	//shift chunks to left (decrease value)
	if(!pos)
		return *this;

	if(pos >= size_)
	{
		std::fill(chunks_.begin(), chunks_.end(), zero_chunk_);
		return *this;
	}

	set_unused_bits_(false);

	const std::size_t left_offset = pos % chunk_size_; 				//left part (smaller) of the chunk will become the right (larger) part of the new chunk
	const std::size_t right_offset = chunk_size_ - left_offset; 	//right part (larger) of the chunk will become the left (smallest) part of the new chunk
	const std::size_t chunk_offset = pos / chunk_size_;				//last chunk to be set in the new bitset

	if(pos % chunk_size_)
	{
		//we will set every chunk AFTER chunk_offset, we will deal with chunk_offset separately as it will only need part of it set
		for(std::size_t itr = 0; itr < chunks_.size() - chunk_offset - 1; ++itr)
			chunks_[itr] = (chunks_[itr + chunk_offset] >> left_offset) | (chunks_[itr + chunk_offset + 1] << right_offset);

		chunks_[chunks_.size() - chunk_offset - 1] = chunks_[chunks_.size() - 1] >> left_offset;
	}
	else
	{
		//full chunk shift
		for(std::size_t itr = 0; itr <= chunks_.size() - chunk_offset - 1; ++itr)
			chunks_[itr] = chunks_[itr + chunk_offset];
	}

	std::fill(chunks_.begin() + chunks_.size() - chunk_offset, chunks_.end(), zero_chunk_);

	return *this;
}


//-------------------------CONVERSIONS-------------------------

template <typename Chunk_T, typename Allocator_T>
template <typename Char_T, typename Traits_T, typename Str_Allocator_T>
constexpr std::basic_string <Char_T, Traits_T, Str_Allocator_T> Bitset<Chunk_T, Allocator_T>::to_string(const Char_T zero, const Char_T one) const
{
	std::basic_string <Char_T, Traits_T, Str_Allocator_T> tmp(size_, zero);

	std::size_t pos = size_;
	for(auto chunk : chunks_)
	{
		for(std::size_t bit_pos = 0; bit_pos < chunk_size_ && pos-- > 0; ++ bit_pos)
		{
			if(chunk & (one_chunk_ << bit_pos))
				Traits_T::assign(tmp[pos], one);
		}
	}

	return tmp;
};


//-------------------------NON MEMBER FUNCTIOS-------------------------

template <typename Chunk_T, typename Allocator_T, typename Char_T, typename Traits_T>
std::basic_ostream <Char_T, Traits_T>& operator << (std::basic_ostream <Char_T, Traits_T>&out, const Bitset <Chunk_T, Allocator_T> &mask)
{
	//i sure hope this is enough to make it portable, must investigate further
	if(!mask.size_)
		return out;

	Char_T zero = std::use_facet <std::ctype <Char_T>>(out.getloc()).widen('0');
	Char_T one = std::use_facet <std::ctype <Char_T>>(out.getloc()).widen('1');

	return out << mask.template to_string <Char_T, Traits_T, std::allocator <Char_T>> (zero, one);
};


//-------------------------private helpers/getters-------------------------

template <typename Chunk_T, typename Allocator_T>
constexpr std::size_t Bitset <Chunk_T, Allocator_T>::needed_chunks_(const std::size_t size) const noexcept
{
	return size / chunk_size_ + (size % chunk_size_ > 0);
};

template <typename Chunk_T, typename Allocator_T>
constexpr std::size_t Bitset <Chunk_T, Allocator_T>::sizeof_chunk_() noexcept
{
	if constexpr (std::is_integral<Chunk_T>::value)
		return sizeof(Chunk_T) * CHAR_BIT;
	else
		return Chunk_T::sizeof_ * CHAR_BIT;
};

template <typename Chunk_T, typename Allocator_T>
constexpr std::string Bitset <Chunk_T, Allocator_T>::oor_msg_builder_(const std::string &which, const std::size_t pos) const
{
	return std::string(		
		"bit::Bitset::"
		+ which
		+ "(): pos (which is " 
		+ std::to_string(pos) 
		+ ") >= size() (which is " 
		+ std::to_string(size_) 
		+ ")"
	);
};

template <typename Chunk_T, typename Allocator_T>
constexpr std::size_t Bitset <Chunk_T, Allocator_T>::chunk_pos_(const std::size_t pos) const noexcept
{
	return pos / chunk_size_;
};

template <typename Chunk_T, typename Allocator_T>
constexpr std::size_t Bitset <Chunk_T, Allocator_T>::bit_pos_(const std::size_t pos) const noexcept
{
	return pos % chunk_size_;
};

template <typename Chunk_T, typename Allocator_T>
void Bitset <Chunk_T, Allocator_T>::set_unused_bits_(const bool value) //noexcept if chunks isnt empty !!!!MAKE SURE THIS WORKS
{
	std::size_t bit_pos = bit_pos_(size_);

	if(bit_pos)
	{
		if(value)
			chunks_.back() |= set_chunk_ << bit_pos; 					// 0000 0111
		else
			chunks_.back() &= ~(set_chunk_ << bit_pos);					// 1111 1000
	}
};


/*
============================================================================
-------------------------class Reference methods BEGIN-------------------------
============================================================================
*/


template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>::Reference::Reference(Bitset <Chunk_T, Allocator_T> &target, const std::size_t pos)
	:
	chunk_reference_(target.chunks_[target.chunk_pos_(pos)]),
	chunk_mask_true_(one_chunk_ << target.bit_pos_(pos)),
	chunk_mask_false_(~(one_chunk_ << target.bit_pos_(pos)))
{};


template <typename Chunk_T, typename Allocator_T>
constexpr Bitset <Chunk_T, Allocator_T>::Reference::operator bool () const noexcept
{
	return chunk_reference_ & chunk_mask_true_;
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference& Bitset <Chunk_T, Allocator_T>::Reference::operator = (const bool value) noexcept
{
	value ? chunk_reference_ |= chunk_mask_true_ : chunk_reference_ &= chunk_mask_false_;

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference& Bitset <Chunk_T, Allocator_T>::Reference::operator = (const Bitset <Chunk_T, Allocator_T>::Reference& rhs) noexcept
{
	rhs ? chunk_reference_ |= chunk_mask_true_ : chunk_reference_ &= chunk_mask_false_;

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference& Bitset <Chunk_T, Allocator_T>::Reference::operator &= (const bool value) noexcept
{
	if(!value) 
		chunk_reference_ &= chunk_mask_false_;

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference& Bitset <Chunk_T, Allocator_T>::Reference::operator |= (const bool value) noexcept
{
	if(value) 
		chunk_reference_ |= chunk_mask_true_;

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr typename Bitset <Chunk_T, Allocator_T>::Reference& Bitset <Chunk_T, Allocator_T>::Reference::operator ^= (const bool value) noexcept
{
	if(value)
		chunk_reference_ ^= chunk_mask_true_;

	return *this;
};

template <typename Chunk_T, typename Allocator_T>
constexpr bool Bitset <Chunk_T, Allocator_T>::Reference::operator ~ () const noexcept
{
	return !(chunk_reference_ & chunk_mask_true_);
};


} //namespace end