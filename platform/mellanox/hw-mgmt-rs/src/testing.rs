//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//! For tests, here and in platform-native: a test root with the modules'
//! imports forgotten.

use std::ops::Deref;

use crate::dpu_thermal_update;

/// Every module back to never imported, as in a new process.
pub fn forget_imports() {
    dpu_thermal_update::forget_import();
}

/// A test root, with the modules' imports forgotten.
pub struct Root(platform_base::testing::Root);

impl Root {
    pub fn new() -> Root {
        let root = platform_base::testing::Root::new();
        forget_imports();
        Root(root)
    }
}

impl Default for Root {
    fn default() -> Self {
        Self::new()
    }
}

impl Deref for Root {
    type Target = platform_base::testing::Root;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}
