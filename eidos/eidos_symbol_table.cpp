//
//  eidos_symbol_table.cpp
//  Eidos
//
//  Created by Ben Haller on 4/12/15.
//  Copyright (c) 2015 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of Eidos.
//
//	Eidos is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	Eidos is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with Eidos.  If not, see <http://www.gnu.org/licenses/>.


#include "eidos_symbol_table.h"
#include "eidos_value.h"
#include "eidos_global.h"
#include "eidos_token.h"
#include "math.h"


using std::string;
using std::vector;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::istream;
using std::ostream;


//
//	EidosSymbolTable
//
#pragma mark EidosSymbolTable

EidosSymbolTable::EidosSymbolTable(EidosSymbolUsageParamBlock *p_symbol_usage, bool p_start_with_hash)
{
	// Set up the symbol table itself
	using_internal_symbols_ = !p_start_with_hash;
	internal_symbol_count_ = 0;
	
	// We statically allocate our base symbols for fast setup / teardown
	static EidosSymbolTableEntry *trueConstant = nullptr;
	static EidosSymbolTableEntry *falseConstant = nullptr;
	static EidosSymbolTableEntry *nullConstant = nullptr;
	static EidosSymbolTableEntry *piConstant = nullptr;
	static EidosSymbolTableEntry *eConstant = nullptr;
	static EidosSymbolTableEntry *infConstant = nullptr;
	static EidosSymbolTableEntry *nanConstant = nullptr;
	
	if (!trueConstant)
	{
		trueConstant = new EidosSymbolTableEntry(gEidosStr_T, gStaticEidosValue_LogicalT);
		falseConstant = new EidosSymbolTableEntry(gEidosStr_F, gStaticEidosValue_LogicalF);
		nullConstant = new EidosSymbolTableEntry(gEidosStr_NULL, gStaticEidosValueNULL);
		piConstant = new EidosSymbolTableEntry(gEidosStr_PI, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(M_PI)));
		eConstant = new EidosSymbolTableEntry(gEidosStr_E, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(M_E)));
		infConstant = new EidosSymbolTableEntry(gEidosStr_INF, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(std::numeric_limits<double>::infinity())));
		nanConstant = new EidosSymbolTableEntry(gEidosStr_NAN, EidosValue_SP(new (gEidosValuePool->AllocateChunk()) EidosValue_Float_singleton(std::numeric_limits<double>::quiet_NaN())));
	}
	
	// We can use InitializeConstantSymbolEntry() here since we obey its requirements (see header)
	if (p_symbol_usage)
	{
		// Include symbols only if they are used by the script we are being created to interpret.
		// Eidos Contexts can check for symbol usage if they wish, for maximal construct/destruct speed.
		// Symbols are defined here from least likely to most likely to be used (from guessing, not metrics),
		// to optimize the symbol table search time; the table is search from last added to first added.
		if (p_symbol_usage->contains_NAN_)
			InitializeConstantSymbolEntry(nanConstant);
		if (p_symbol_usage->contains_INF_)
			InitializeConstantSymbolEntry(infConstant);
		if (p_symbol_usage->contains_PI_)
			InitializeConstantSymbolEntry(piConstant);
		if (p_symbol_usage->contains_E_)
			InitializeConstantSymbolEntry(eConstant);
		if (p_symbol_usage->contains_NULL_)
			InitializeConstantSymbolEntry(nullConstant);
		if (p_symbol_usage->contains_F_)
			InitializeConstantSymbolEntry(falseConstant);
		if (p_symbol_usage->contains_T_)
			InitializeConstantSymbolEntry(trueConstant);
	}
	else
	{
		InitializeConstantSymbolEntry(nanConstant);
		InitializeConstantSymbolEntry(infConstant);
		InitializeConstantSymbolEntry(piConstant);
		InitializeConstantSymbolEntry(eConstant);
		InitializeConstantSymbolEntry(nullConstant);
		InitializeConstantSymbolEntry(falseConstant);
		InitializeConstantSymbolEntry(trueConstant);
	}
}

EidosSymbolTable::~EidosSymbolTable(void)
{
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			// free symbol names that we own
			if (!symbol_slot->symbol_name_externally_owned_)
				delete symbol_slot->symbol_name_;
		}
	}
}

std::vector<std::string> EidosSymbolTable::ReadOnlySymbols(void) const
{
	std::vector<std::string> symbol_names;
	
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			const EidosSymbolTable_InternalSlot &symbol_slot = internal_symbols_[symbol_index];
			
			if (symbol_slot.symbol_is_const_)
				symbol_names.push_back(*symbol_slot.symbol_name_);
		}
	}
	else
	{
		for (auto symbol_slot_iter = hash_symbols_.begin(); symbol_slot_iter != hash_symbols_.end(); ++symbol_slot_iter)
		{
			const EidosSymbolTable_ExternalSlot &symbol_slot = symbol_slot_iter->second;
			
			if (symbol_slot.symbol_is_const_)
				symbol_names.push_back(symbol_slot_iter->first);
		}
	}
	
	return symbol_names;
}

std::vector<std::string> EidosSymbolTable::ReadWriteSymbols(void) const
{
	std::vector<std::string> symbol_names;
	
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			const EidosSymbolTable_InternalSlot &symbol_slot = internal_symbols_[symbol_index];
			
			if (!symbol_slot.symbol_is_const_)
				symbol_names.push_back(*symbol_slot.symbol_name_);
		}
	}
	else
	{
		for (auto symbol_slot_iter = hash_symbols_.begin(); symbol_slot_iter != hash_symbols_.end(); ++symbol_slot_iter)
		{
			const EidosSymbolTable_ExternalSlot &symbol_slot = symbol_slot_iter->second;
			
			if (!symbol_slot.symbol_is_const_)
				symbol_names.push_back(symbol_slot_iter->first);
		}
	}
	
	return symbol_names;
}

EidosValue_SP EidosSymbolTable::GetValueOrRaiseForToken(const EidosToken *p_symbol_token) const
{
	if (using_internal_symbols_)
	{
		const std::string &symbol_name = p_symbol_token->token_string_;
		int key_length = (int)symbol_name.size();
		const char *symbol_name_data = symbol_name.data();
		
		// This is the same logic as _SlotIndexForSymbol, but it is repeated here for speed; getting values should be super fast
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			// use the length of the string to make the scan fast; only compare for equal-length strings
			if (symbol_slot->symbol_name_length_ == key_length)
			{
				// The logic here is equivalent to symbol_slot->symbol_name_->compare(symbol_name), but runs much faster
				const char *slot_name_data = symbol_slot->symbol_name_data_;
				int char_index;
				
				for (char_index = 0; char_index < key_length; ++char_index)
					if (slot_name_data[char_index] != symbol_name_data[char_index])
						break;
				
				if (char_index == key_length)
					return symbol_slot->symbol_value_SP_;
			}
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_token->token_string_);
		
		if (symbol_slot_iter != hash_symbols_.end())
			return symbol_slot_iter->second.symbol_value_SP_;
	}
	
	//std::cerr << "ValueForIdentifier: Symbol table: " << *this;
	//std::cerr << "Symbol returned for identifier " << p_identifier << " == (" << result->Type() << ") " << *result << endl;
	
	EIDOS_TERMINATION << "ERROR (EidosSymbolTable::GetValueOrRaiseForToken): undefined identifier " << p_symbol_token->token_string_ << "." << eidos_terminate(p_symbol_token);
}

EidosValue_SP EidosSymbolTable::GetNonConstantValueOrRaiseForToken(const EidosToken *p_symbol_token) const
{
	if (using_internal_symbols_)
	{
		const std::string &symbol_name = p_symbol_token->token_string_;
		int key_length = (int)symbol_name.size();
		const char *symbol_name_data = symbol_name.data();
		
		// This is the same logic as _SlotIndexForSymbol, but it is repeated here for speed; getting values should be super fast
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			// use the length of the string to make the scan fast; only compare for equal-length strings
			if (symbol_slot->symbol_name_length_ == key_length)
			{
				// The logic here is equivalent to symbol_slot->symbol_name_->compare(symbol_name), but runs much faster
				const char *slot_name_data = symbol_slot->symbol_name_data_;
				int char_index;
				
				for (char_index = 0; char_index < key_length; ++char_index)
					if (slot_name_data[char_index] != symbol_name_data[char_index])
						break;
				
				if (char_index == key_length)
				{
					if (symbol_slot->symbol_is_const_)
						EIDOS_TERMINATION << "ERROR (EidosSymbolTable::GetNonConstantValueOrRaiseForToken): identifier " << symbol_name << " is a constant." << eidos_terminate(p_symbol_token);
					
					return symbol_slot->symbol_value_SP_;
				}
			}
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_token->token_string_);
		
		if (symbol_slot_iter != hash_symbols_.end())
		{
			const EidosSymbolTable_ExternalSlot &symbol_slot = symbol_slot_iter->second;
			
			if (symbol_slot.symbol_is_const_)
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::GetNonConstantValueOrRaiseForToken): identifier " << p_symbol_token->token_string_ << " is a constant." << eidos_terminate(p_symbol_token);
			
			return symbol_slot.symbol_value_SP_;
		}
	}
	
	//std::cerr << "ValueForIdentifier: Symbol table: " << *this;
	//std::cerr << "Symbol returned for identifier " << p_identifier << " == (" << result->Type() << ") " << *result << endl;
	
	EIDOS_TERMINATION << "ERROR (EidosSymbolTable::GetValueOrRaiseForToken): undefined identifier " << p_symbol_token->token_string_ << "." << eidos_terminate(p_symbol_token);
}

EidosValue_SP EidosSymbolTable::GetValueOrRaiseForSymbol(const std::string &p_symbol_name) const
{
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		
		// This is the same logic as _SlotIndexForSymbol, but it is repeated here for speed; getting values should be super fast
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			// use the length of the string to make the scan fast; only call compare() for equal-length strings
			if (symbol_slot->symbol_name_length_ == key_length)
			{
				// The logic here is equivalent to symbol_slot->symbol_name_->compare(p_symbol_name), but runs much faster
				const char *slot_name_data = symbol_slot->symbol_name_data_;
				int char_index;
				
				for (char_index = 0; char_index < key_length; ++char_index)
					if (slot_name_data[char_index] != symbol_name_data[char_index])
						break;
				
				if (char_index == key_length)
					return symbol_slot->symbol_value_SP_;
			}
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_name);
		
		if (symbol_slot_iter != hash_symbols_.end())
			return symbol_slot_iter->second.symbol_value_SP_;
	}
	
	//std::cerr << "ValueForIdentifier: Symbol table: " << *this;
	//std::cerr << "Symbol returned for identifier " << p_identifier << " == (" << result->Type() << ") " << *result << endl;
	
	EIDOS_TERMINATION << "ERROR (EidosSymbolTable::GetValueOrRaiseForSymbol): undefined identifier " << p_symbol_name << "." << eidos_terminate(nullptr);
}

EidosValue_SP EidosSymbolTable::GetValueOrNullForSymbol(const std::string &p_symbol_name) const
{
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		
		// This is the same logic as _SlotIndexForSymbol, but it is repeated here for speed; getting values should be super fast
		for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
		{
			const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
			
			// use the length of the string to make the scan fast; only call compare() for equal-length strings
			if (symbol_slot->symbol_name_length_ == key_length)
			{
				// The logic here is equivalent to symbol_slot->symbol_name_->compare(p_symbol_name), but runs much faster
				const char *slot_name_data = symbol_slot->symbol_name_data_;
				int char_index;
				
				for (char_index = 0; char_index < key_length; ++char_index)
					if (slot_name_data[char_index] != symbol_name_data[char_index])
						break;
				
				if (char_index == key_length)
					return symbol_slot->symbol_value_SP_;
			}
		}
	}
	else
	{
		auto symbol_slot_iter = hash_symbols_.find(p_symbol_name);
		
		if (symbol_slot_iter != hash_symbols_.end())
			return symbol_slot_iter->second.symbol_value_SP_;
	}
	
	return EidosValue_SP(nullptr);
}

// does a fast search for the slot matching the search key; returns -1 if no match is found
int EidosSymbolTable::_SlotIndexForSymbol(int p_symbol_name_length, const char *p_symbol_name_data) const
{
	// search through the symbol table in reverse order, most-recently-defined symbols first
	for (int symbol_index = (int)internal_symbol_count_ - 1; symbol_index >= 0; --symbol_index)
	{
		const EidosSymbolTable_InternalSlot *symbol_slot = internal_symbols_ + symbol_index;
		
		// use the length of the string to make the scan fast; only call compare() for equal-length strings
		if (symbol_slot->symbol_name_length_ == p_symbol_name_length)
		{
			// The logic here is equivalent to symbol_slot->symbol_name_->compare(p_symbol_name), but runs much faster
			// since most symbols will fail the comparison on the first character, avoiding the function call and setup
			const char *slot_name_data = symbol_slot->symbol_name_data_;
			int char_index;
			
			for (char_index = 0; char_index < p_symbol_name_length; ++char_index)
				if (slot_name_data[char_index] != p_symbol_name_data[char_index])
					break;
			
			if (char_index == p_symbol_name_length)
				return symbol_index;
		}
	}
	
	return -1;
}

void EidosSymbolTable::_SwitchToHash(void)
{
	if (using_internal_symbols_)
	{
		for (size_t symbol_index = 0; symbol_index < internal_symbol_count_; ++symbol_index)
		{
			EidosSymbolTable_InternalSlot &old_symbol_slot = internal_symbols_[symbol_index];
			EidosSymbolTable_ExternalSlot new_symbol_slot;
			
			new_symbol_slot.symbol_value_SP_ = std::move(old_symbol_slot.symbol_value_SP_);
			new_symbol_slot.symbol_is_const_ = old_symbol_slot.symbol_is_const_;
			
			hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(*old_symbol_slot.symbol_name_, std::move(new_symbol_slot)));
			
			// clean up the built-in slot; probably unnecessary, but prevents hard-to-find bugs
			old_symbol_slot.symbol_value_SP_.reset();
			if (old_symbol_slot.symbol_name_externally_owned_)
				delete old_symbol_slot.symbol_name_;
			old_symbol_slot.symbol_name_ = nullptr;
			old_symbol_slot.symbol_name_data_ = nullptr;
		}
		
		using_internal_symbols_ = false;
		internal_symbol_count_ = 0;
	}
}

void EidosSymbolTable::SetValueForSymbol(const std::string &p_symbol_name, EidosValue_SP p_value)
{
	// if it's invisible then we copy it, since the symbol table never stores invisible values
	if (p_value->Invisible())
		p_value = p_value->CopyValues();
	
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		int symbol_slot = _SlotIndexForSymbol(key_length, symbol_name_data);
		
		if (symbol_slot == -1)
		{
			if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
			{
				EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
				std::string *string_copy = new std::string(p_symbol_name);
				const char *string_copy_data_ = string_copy->data();
				
				new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
				new_symbol_slot_ptr->symbol_name_ = string_copy;
				new_symbol_slot_ptr->symbol_name_data_ = string_copy_data_;
				new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)key_length;
				new_symbol_slot_ptr->symbol_name_externally_owned_ = false;
				new_symbol_slot_ptr->symbol_is_const_ = false;
				
				++internal_symbol_count_;
				return;
			}
			
			_SwitchToHash();
		}
		else
		{
			EidosSymbolTable_InternalSlot *existing_symbol_slot_ptr = internal_symbols_ + symbol_slot;
			
			if (existing_symbol_slot_ptr->symbol_is_const_)
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << p_symbol_name << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
			
			// We replace the existing symbol value, of course.  Everything else gets inherited, since we're replacing the value in an existing slot;
			// we can continue using the same symbol name, name length, constness (since that is guaranteed to be false here), etc.
			existing_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
			return;
		}
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(p_symbol_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// the symbol is not yet defined
		EidosSymbolTable_ExternalSlot new_symbol_slot;
		
		new_symbol_slot.symbol_value_SP_ = std::move(p_value);
		new_symbol_slot.symbol_is_const_ = false;
		
		hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(p_symbol_name, std::move(new_symbol_slot)));
	}
	else
	{
		// if the key was already defined, replace its value
		if (existing_symbol_slot_iter->second.symbol_is_const_)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetValueForSymbol): identifier '" << p_symbol_name << "' cannot be redefined because it is a constant." << eidos_terminate(nullptr);
		
		existing_symbol_slot_iter->second.symbol_value_SP_ = std::move(p_value);
	}
}

void EidosSymbolTable::SetConstantForSymbol(const std::string &p_symbol_name, EidosValue_SP p_value)
{
	// if it's invisible then we copy it, since the symbol table never stores invisible values
	if (p_value->Invisible())
		p_value = p_value->CopyValues();
	
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		int symbol_slot = _SlotIndexForSymbol(key_length, symbol_name_data);
		
		// can't already be defined as either a constant or a variable; if you want to define a constant, you have to get there first
		if (symbol_slot >= 0)
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetConstantForSymbol): (internal error) identifier '" << p_symbol_name << "' is already defined." << eidos_terminate(nullptr);
		
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			std::string *string_copy = new std::string(p_symbol_name);
			const char *string_copy_data_ = string_copy->data();
			
			new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
			new_symbol_slot_ptr->symbol_name_ = string_copy;
			new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)key_length;
			new_symbol_slot_ptr->symbol_name_data_ = string_copy_data_;
			new_symbol_slot_ptr->symbol_name_externally_owned_ = false;
			new_symbol_slot_ptr->symbol_is_const_ = true;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(p_symbol_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// the symbol is not yet defined
		EidosSymbolTable_ExternalSlot new_symbol_slot;
		
		new_symbol_slot.symbol_value_SP_ = std::move(p_value);
		new_symbol_slot.symbol_is_const_ = true;
		
		hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(p_symbol_name, std::move(new_symbol_slot)));
	}
	else
	{
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::SetConstantForSymbol): (internal error) identifier '" << p_symbol_name << "' is already defined." << eidos_terminate(nullptr);
	}
}

void EidosSymbolTable::RemoveValueForSymbol(const std::string &p_symbol_name, bool p_remove_constant)
{
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		int symbol_slot = _SlotIndexForSymbol(key_length, symbol_name_data);
		
		if (symbol_slot >= 0)
		{
			EidosSymbolTable_InternalSlot *existing_symbol_slot_ptr = internal_symbols_ + symbol_slot;
			
			if (existing_symbol_slot_ptr->symbol_is_const_ && !p_remove_constant)
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::RemoveValueForSymbol): identifier '" << p_symbol_name << "' is a constant and thus cannot be removed." << eidos_terminate(nullptr);
			
			// clean up the slot being removed
			existing_symbol_slot_ptr->symbol_value_SP_.reset();
			
			if (!existing_symbol_slot_ptr->symbol_name_externally_owned_)
				delete existing_symbol_slot_ptr->symbol_name_;
			existing_symbol_slot_ptr->symbol_name_ = nullptr;
			existing_symbol_slot_ptr->symbol_name_data_ = nullptr;
			
			// remove the slot from the vector
			EidosSymbolTable_InternalSlot *backfill_symbol_slot_ptr = internal_symbols_ + (--internal_symbol_count_);
			
			if (existing_symbol_slot_ptr != backfill_symbol_slot_ptr)
			{
				*existing_symbol_slot_ptr = std::move(*backfill_symbol_slot_ptr);
				
				// clean up the backfill slot; probably unnecessary, but prevents hard-to-find bugs
				backfill_symbol_slot_ptr->symbol_value_SP_.reset();
				backfill_symbol_slot_ptr->symbol_name_ = nullptr;
				backfill_symbol_slot_ptr->symbol_name_data_ = nullptr;
			}
		}
	}
	else
	{
		hash_symbols_.erase(p_symbol_name);
	}
}

void EidosSymbolTable::InitializeConstantSymbolEntry(EidosSymbolTableEntry *p_new_entry)
{
#ifdef DEBUG
	if (p_new_entry->second->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::InitializeConstantSymbolEntry): (internal error) this method should be called only for non-invisible objects." << eidos_terminate(nullptr);
#endif
	
	// we assume that this symbol is not yet defined, for maximal set-up speed
	const std::string &entry_name = p_new_entry->first;
	
	if (using_internal_symbols_)
	{
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			
			new_symbol_slot_ptr->symbol_value_SP_ = p_new_entry->second;
			new_symbol_slot_ptr->symbol_name_ = &entry_name;		// take a pointer to the external object, which must live longer than us!
			new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)entry_name.size();
			new_symbol_slot_ptr->symbol_name_data_ = entry_name.data();
			new_symbol_slot_ptr->symbol_name_externally_owned_ = true;
			new_symbol_slot_ptr->symbol_is_const_ = true;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	EidosSymbolTable_ExternalSlot new_symbol_slot;
	
	new_symbol_slot.symbol_value_SP_ = p_new_entry->second;
	new_symbol_slot.symbol_is_const_ = true;
	
	hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(entry_name, std::move(new_symbol_slot)));
}

void EidosSymbolTable::InitializeConstantSymbolEntry(const std::string &p_symbol_name, EidosValue_SP p_value)
{
#ifdef DEBUG
	if (p_value->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::InitializeConstantSymbolEntry): (internal error) this method should be called only for non-invisible objects." << eidos_terminate(nullptr);
#endif
	
	// we assume that this symbol is not yet defined, for maximal set-up speed
	if (using_internal_symbols_)
	{
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			
			new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
			new_symbol_slot_ptr->symbol_name_ = &p_symbol_name;		// take a pointer to the external object, which must live longer than us!
			new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)p_symbol_name.size();
			new_symbol_slot_ptr->symbol_name_data_ = p_symbol_name.data();
			new_symbol_slot_ptr->symbol_name_externally_owned_ = true;
			new_symbol_slot_ptr->symbol_is_const_ = true;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	EidosSymbolTable_ExternalSlot new_symbol_slot;
	
	new_symbol_slot.symbol_value_SP_ = std::move(p_value);
	new_symbol_slot.symbol_is_const_ = true;
	
	hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(p_symbol_name, std::move(new_symbol_slot)));
}

void EidosSymbolTable::ReinitializeConstantSymbolEntry(EidosSymbolTableEntry *p_new_entry)
{
#ifdef DEBUG
	if (p_new_entry->second->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) this method should be called only for non-invisible objects." << eidos_terminate(nullptr);
#endif
	
	// check whether the symbol is already defined; if so, it should be identical or we raise
	const std::string &entry_name = p_new_entry->first;
	
	if (using_internal_symbols_)
	{
		int key_length = (int)entry_name.size();
		const char *symbol_name_data = entry_name.data();
		int symbol_slot = _SlotIndexForSymbol(key_length, symbol_name_data);
		
		if (symbol_slot >= 0)
		{
			EidosSymbolTable_InternalSlot *old_slot = internal_symbols_ + symbol_slot;
			
			if ((!old_slot->symbol_is_const_) || (old_slot->symbol_value_SP_.get() != p_new_entry->second.get()))
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) identifier '" << entry_name << "' is already defined, but the existing entry does not match." << eidos_terminate(nullptr);
			
			// a matching slot already exists, so we can just return
			return;
		}
		
		// ok, it is not defined so we need to define it
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			
			new_symbol_slot_ptr->symbol_value_SP_ = p_new_entry->second;
			new_symbol_slot_ptr->symbol_name_ = &entry_name;		// take a pointer to the external object, which must live longer than us!
			new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)entry_name.size();
			new_symbol_slot_ptr->symbol_name_data_ = entry_name.data();
			new_symbol_slot_ptr->symbol_name_externally_owned_ = true;
			new_symbol_slot_ptr->symbol_is_const_ = true;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(entry_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// the symbol is not yet defined
		EidosSymbolTable_ExternalSlot new_symbol_slot;
		
		new_symbol_slot.symbol_value_SP_ = p_new_entry->second;
		new_symbol_slot.symbol_is_const_ = true;
		
		hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(entry_name, std::move(new_symbol_slot)));
	}
	else
	{
		EidosSymbolTable_ExternalSlot &old_slot = existing_symbol_slot_iter->second;
		
		if ((!old_slot.symbol_is_const_) || (old_slot.symbol_value_SP_.get() != p_new_entry->second.get()))
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) identifier '" << entry_name << "' is already defined, but the existing entry does not match." << eidos_terminate(nullptr);
	}
}

void EidosSymbolTable::ReinitializeConstantSymbolEntry(const std::string &p_symbol_name, EidosValue_SP p_value)
{
#ifdef DEBUG
	if (p_value->Invisible())
		EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) this method should be called only for non-invisible objects." << eidos_terminate(nullptr);
#endif
	
	// check whether the symbol is already defined; if so, it should be identical or we raise
	if (using_internal_symbols_)
	{
		int key_length = (int)p_symbol_name.size();
		const char *symbol_name_data = p_symbol_name.data();
		int symbol_slot = _SlotIndexForSymbol(key_length, symbol_name_data);
		
		if (symbol_slot >= 0)
		{
			EidosSymbolTable_InternalSlot *old_slot = internal_symbols_ + symbol_slot;
			
			if ((!old_slot->symbol_is_const_) || (!old_slot->symbol_name_externally_owned_) || (old_slot->symbol_value_SP_.get() != p_value.get()))
				EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) identifier '" << p_symbol_name << "' is already defined, but the existing entry does not match." << eidos_terminate(nullptr);
			
			// a matching slot already exists, so we can just return
			return;
		}
		
		// ok, it is not defined so we need to define it
		if (internal_symbol_count_ < EIDOS_SYMBOL_TABLE_BASE_SIZE)
		{
			EidosSymbolTable_InternalSlot *new_symbol_slot_ptr = internal_symbols_ + internal_symbol_count_;
			
			new_symbol_slot_ptr->symbol_value_SP_ = std::move(p_value);
			new_symbol_slot_ptr->symbol_name_ = &p_symbol_name;		// take a pointer to the external object, which must live longer than us!
			new_symbol_slot_ptr->symbol_name_length_ = (uint16_t)p_symbol_name.size();
			new_symbol_slot_ptr->symbol_name_data_ = p_symbol_name.data();
			new_symbol_slot_ptr->symbol_name_externally_owned_ = true;
			new_symbol_slot_ptr->symbol_is_const_ = true;
			
			++internal_symbol_count_;
			return;
		}
		
		_SwitchToHash();
	}
	
	// fall-through to the hash table case
	auto existing_symbol_slot_iter = hash_symbols_.find(p_symbol_name);
	
	if (existing_symbol_slot_iter == hash_symbols_.end())
	{
		// the symbol is not yet defined
		EidosSymbolTable_ExternalSlot new_symbol_slot;
		
		new_symbol_slot.symbol_value_SP_ = std::move(p_value);
		new_symbol_slot.symbol_is_const_ = true;
		
		hash_symbols_.insert(std::pair<std::string, EidosSymbolTable_ExternalSlot>(p_symbol_name, std::move(new_symbol_slot)));
	}
	else
	{
		EidosSymbolTable_ExternalSlot &old_slot = existing_symbol_slot_iter->second;
		
		if ((!old_slot.symbol_is_const_) || (old_slot.symbol_value_SP_.get() != p_value.get()))
			EIDOS_TERMINATION << "ERROR (EidosSymbolTable::ReinitializeConstantSymbolEntry): (internal error) identifier '" << p_symbol_name << "' is already defined, but the existing entry does not match." << eidos_terminate(nullptr);
	}
}

std::ostream &operator<<(std::ostream &p_outstream, const EidosSymbolTable &p_symbols)
{
	std::vector<std::string> read_only_symbol_names = p_symbols.ReadOnlySymbols();
	std::vector<std::string> read_write_symbol_names = p_symbols.ReadWriteSymbols();
	std::vector<std::string> symbol_names;
	
	symbol_names.insert(symbol_names.end(), read_only_symbol_names.begin(), read_only_symbol_names.end());
	symbol_names.insert(symbol_names.end(), read_write_symbol_names.begin(), read_write_symbol_names.end());
	std::sort(symbol_names.begin(), symbol_names.end());
	
	for (auto symbol_name_iter = symbol_names.begin(); symbol_name_iter != symbol_names.end(); ++symbol_name_iter)
	{
		const std::string &symbol_name = *symbol_name_iter;
		EidosValue_SP symbol_value = p_symbols.GetValueOrRaiseForSymbol(symbol_name);
		int symbol_count = symbol_value->Count();
		bool is_const = std::find(read_only_symbol_names.begin(), read_only_symbol_names.end(), symbol_name) != read_only_symbol_names.end();
		
		if (symbol_count <= 2)
			p_outstream << symbol_name << (is_const ? " => (" : " -> (") << symbol_value->Type() << ") " << *symbol_value << endl;
		else
		{
			EidosValue_SP first_value = symbol_value->GetValueAtIndex(0, nullptr);
			EidosValue_SP second_value = symbol_value->GetValueAtIndex(1, nullptr);
			
			p_outstream << symbol_name << (is_const ? " => (" : " -> (") << symbol_value->Type() << ") " << *first_value << " " << *second_value << " ... (" << symbol_count << " values)" << endl;
		}
	}
	
	return p_outstream;
}




































































