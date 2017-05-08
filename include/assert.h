/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __ASSERT_H
#define __ASSERT_H

extern void assert_handler(char *file, uint16_t line_num);

#define ASSERT(expr) if (!(expr)) { assert_handler(__FILE__, __LINE__); } 

#endif