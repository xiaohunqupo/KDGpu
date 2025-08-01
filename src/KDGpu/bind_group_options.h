/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include <KDGpu/bind_group_description.h>
#include <KDGpu/bind_group_layout.h>

#include <string_view>
#include <vector>

namespace KDGpu {

struct BindGroupPool_t;

struct BindGroupEntry { // An entry into a BindGroup ( == a descriptor in a descriptor set)
    uint32_t binding;
    BindingResource resource;
    uint32_t arrayElement{ 0 };
};

struct BindGroupOptions {
    std::string_view label;
    Handle<BindGroupLayout_t> layout;
    std::vector<BindGroupEntry> resources;
    uint32_t maxVariableArrayLength{ 0 };
    Handle<BindGroupPool_t> bindGroupPool;
    bool implicitFree{ true }; // If true, the bind group will be automatically freed when going out of scope. If false, BindGroup will not be released against the Pool (even if going out of scope). The Pool will have to be reset or destroyed for underlying BindGroup to be released.
};

} // namespace KDGpu
