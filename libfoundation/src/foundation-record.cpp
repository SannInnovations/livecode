/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <foundation.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCRecordCreate(MCTypeInfoRef p_typeinfo, const MCValueRef *p_values, uindex_t p_value_count, MCRecordRef& r_record)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    if (p_value_count != t_resolved_typeinfo -> record . field_count)
        return false;
    
    for(uindex_t i = 0; i < p_value_count; i++)
        if (MCTypeInfoConforms(MCValueGetTypeInfo(p_values[i]), t_resolved_typeinfo -> record . fields[i] . type))
            return false;
    
    __MCRecord *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeRecord, self);
    
    if (t_success)
        t_success = MCMemoryNewArray(p_value_count, self -> fields);
    
    if (t_success)
    {
        for(uindex_t i = 0; i < p_value_count; i++)
            self -> fields[i] = MCValueRetain(p_values[i]);
    
        self -> typeinfo = MCValueRetain(p_typeinfo);
    
        r_record = self;
    }
    else
    {
        MCMemoryDeleteArray(self -> fields);
        MCMemoryDelete(self);
    }
    
    return t_success;
    
}

bool MCRecordCreateMutable(MCTypeInfoRef p_typeinfo, MCRecordRef& r_record)
{
    bool t_success;
    t_success = true;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(p_typeinfo);
    
    __MCRecord *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeRecord, self);
    
    if (t_success)
        t_success = MCMemoryNewArray(t_resolved_typeinfo -> record . field_count, self -> fields);
    
    if (t_success)
    {
        for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
            self -> fields[i] = MCValueRetain(kMCNull);
        
        self -> flags |= kMCRecordFlagIsMutable;
        
        r_record = self;
    }
    else
    {
        MCMemoryDeleteArray(self -> fields);
        MCMemoryDelete(self);
    }
    
    return t_success;
}

bool MCRecordCopy(MCRecordRef self, MCRecordRef& r_new_record)
{
    if (!MCRecordIsMutable(self))
    {
        MCValueRetain(self);
        r_new_record = self;
        return true;
    }
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    return MCRecordCreate(self -> typeinfo, self -> fields, t_resolved_typeinfo -> record . field_count, r_new_record);
}

bool MCRecordCopyAndRelease(MCRecordRef self, MCRecordRef& r_new_record)
{
    // If the MCRecord is immutable we just pass it through (as we are releasing it).
    if (!MCRecordIsMutable(self))
    {
        r_new_record = self;
        return true;
    }
    
    // If the reference is 1, convert it to an immutable MCData
    if (self->references == 1)
    {
        self->flags &= ~kMCRecordFlagIsMutable;
        r_new_record = self;
        return true;
    }
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    // Otherwise make a copy of the data and then release the original
    bool t_success;
    t_success = MCRecordCreate(self -> typeinfo, self -> fields, t_resolved_typeinfo -> record . field_count, r_new_record);
    MCValueRelease(self);
    
    return t_success;
}

bool MCRecordMutableCopy(MCRecordRef self, MCRecordRef& r_new_record)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    MCRecordRef t_new_self;
    if (!MCRecordCreate(self -> typeinfo, self -> fields, t_resolved_typeinfo -> record . field_count, t_new_self))
        return false;
    
    t_new_self -> flags |= kMCRecordFlagIsMutable;
    
    r_new_record = t_new_self;
    
    return true;
}

bool MCRecordMutableCopyAndRelease(MCRecordRef self, MCRecordRef& r_new_record)
{
    if (MCRecordMutableCopy(self, r_new_record))
    {
        MCValueRelease(self);
        return true;
    }
    
    return false;
}

bool MCRecordIsMutable(MCRecordRef self)
{
    return (self -> flags & kMCRecordFlagIsMutable) != 0;
}

bool MCRecordFetchValue(MCRecordRef self, MCNameRef p_field, MCValueRef& r_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualTo(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            r_value = self -> fields[i];
            return true;
        }
    
    return false;
}

bool MCRecordStoreValue(MCRecordRef self, MCNameRef p_field, MCValueRef p_value)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (MCNameIsEqualTo(p_field, t_resolved_typeinfo -> record . fields[i] . name))
        {
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_resolved_typeinfo -> record . fields[i] . type))
                return false;
            
            self -> fields[i] = MCValueRetain(p_value);
            return true;
        }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void __MCRecordDestroy(__MCRecord *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        MCValueRelease(self -> fields[i]);
    MCMemoryDelete(self -> fields);
}

hash_t __MCRecordHash(__MCRecord *self)
{
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    hash_t t_hash;
    t_hash = 0;
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
    {
        hash_t t_element_hash;
        t_element_hash = MCValueHash(self -> fields[i]);
        t_hash = MCHashBytesStream(t_hash, &t_element_hash, sizeof(hash_t));
    }
    return t_hash;
}

bool __MCRecordIsEqualTo(__MCRecord *self, __MCRecord *other_self)
{
    // Records must have the same typeinfo to be equal.
    if (self -> typeinfo != other_self -> typeinfo)
        return false;
    
    MCTypeInfoRef t_resolved_typeinfo;
    t_resolved_typeinfo = __MCTypeInfoResolve(self -> typeinfo);
    
    // Each field within the record must be equal to be equal.
    for(uindex_t i = 0; i < t_resolved_typeinfo -> record . field_count; i++)
        if (!MCValueIsEqualTo(self -> fields[i], other_self -> fields[i]))
            return false;

    return true;
}

bool __MCRecordCopyDescription(__MCRecord *self, MCStringRef &r_description)
{
    return false;
}

bool __MCRecordImmutableCopy(__MCRecord *self, bool p_release, __MCRecord *&r_immutable_value)
{
    if (!p_release)
        return MCRecordCopy(self, r_immutable_value);
    return MCRecordCopyAndRelease(self, r_immutable_value);
}

bool __MCRecordInitialize(void)
{
	return true;
}

void __MCRecordFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////