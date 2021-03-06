// RUN: %clang_cc1 %s -emit-llvm -o - | FileCheck %s -check-prefix=CHECK-DEFAULT
// RUN: %clang_cc1 %s -fno-common -emit-llvm -o - | FileCheck %s -check-prefix=CHECK-NOCOMMON

// CHECK-DEFAULT: @x = common global
// CHECK-NOCOMMON: @x = global
int x;

// CHECK-DEFAULT: @ABC = global
// CHECK-NOCOMMON: @ABC = global
typedef void* (*fn_t)(long a, long b, char *f, int c);
fn_t ABC __attribute__ ((nocommon));

// CHECK-DEFAULT: @y = common global
// CHECK-NOCOMMON: @y = common global
int y __attribute__((common));