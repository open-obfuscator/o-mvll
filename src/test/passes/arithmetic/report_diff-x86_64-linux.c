// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/report_diff.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck %s

// CHECK: omvll::Arithmetic applied obfuscation
// CHECK: --- original
// CHECK: +++ obfuscated
// CHECK: @@ -25,11 +25,11 @@
// CHECK:    %12 = getelementptr inbounds i8, ptr %1, i64 %11
// CHECK:    %13 = load i8, ptr %12, align 1, !tbaa !5
// CHECK:    %14 = sext i8 %13 to i32
// CHECK: -  %15 = xor i32 %14, 35
// CHECK: +  %15 = call i32 @__omvll_mba(i32 %14, i32 35)
// CHECK:    %16 = trunc i32 %15 to i8
// CHECK:    %17 = getelementptr inbounds i8, ptr %0, i64 %11
// CHECK:    store i8 %16, ptr %17, align 1, !tbaa !5
// CHECK: -  %18 = add i32 %5, 1
// CHECK: +  %18 = call i32 @__omvll_mba.1(i32 %5, i32 1)
// CHECK:    br label %4, !llvm.loop !8
// CHECK:  }
//
// CHECK: @@ -39,8 +39,60 @@
// CHECK:  ; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
// CHECK:  declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1
//
// CHECK: +; Function Attrs: alwaysinline optnone
// CHECK: +define private i32 @__omvll_mba(i32 %0, i32 %1) #2 {
// CHECK: +  %3 = add i32 %0, %1
// CHECK: +  %4 = add i32 %3, 1
// CHECK: +  %5 = xor i32 %0, -1
// CHECK: +  %6 = xor i32 %1, -1
// CHECK: +  %7 = or i32 %5, %6
// CHECK: +  %8 = add i32 %4, %7
// CHECK: +  %9 = or i32 %0, %1
// CHECK: +  %10 = add i32 %0, %1
// CHECK: +  %11 = or i32 %0, %1
// CHECK: +  %12 = sub i32 %10, %11
// CHECK: +  %13 = and i32 %0, %1
// CHECK: +  %14 = sub i32 0, %12
// CHECK: +  %15 = xor i32 %8, %14
// CHECK: +  %16 = sub i32 0, %12
// CHECK: +  %17 = and i32 %8, %16
// CHECK: +  %18 = mul i32 2, %17
// CHECK: +  %19 = add i32 %15, %18
// CHECK: +  %20 = sub i32 %8, %12
// CHECK: +  %21 = or i32 %0, %1
// CHECK: +  %22 = and i32 %0, %1
// CHECK: +  %23 = sub i32 %21, %22
// CHECK: +  %24 = xor i32 %0, %1
// CHECK: +  ret i32 %19
// CHECK: +}
// CHECK: +
// CHECK: +; Function Attrs: alwaysinline optnone
// CHECK: +define private i32 @__omvll_mba.1(i32 %0, i32 %1) #2 {
// CHECK: +  %3 = add i32 %0, %1
// CHECK: +  %4 = or i32 %0, %1
// CHECK: +  %5 = sub i32 %3, %4
// CHECK: +  %6 = and i32 %0, %1
// CHECK: +  %7 = add i32 %0, %1
// CHECK: +  %8 = add i32 %7, 1
// CHECK: +  %9 = xor i32 %0, -1
// CHECK: +  %10 = xor i32 %1, -1
// CHECK: +  %11 = or i32 %9, %10
// CHECK: +  %12 = add i32 %8, %11
// CHECK: +  %13 = or i32 %0, %1
// CHECK: +  %14 = and i32 %5, %12
// CHECK: +  %15 = or i32 %5, %12
// CHECK: +  %16 = add i32 %14, %15
// CHECK: +  %17 = add i32 %5, %12
// CHECK: +  %18 = and i32 %0, %1
// CHECK: +  %19 = or i32 %0, %1
// CHECK: +  %20 = add i32 %18, %19
// CHECK: +  %21 = add i32 %0, %1
// CHECK: +  ret i32 %16
// CHECK: +}
// CHECK: +
// CHECK:  attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
// CHECK:  attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
// CHECK: +attributes #2 = { alwaysinline optnone }
//
// CHECK:  !llvm.module.flags = !{!0, !1, !2, !3}
// CHECK:  !llvm.ident = !{!4}

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}
