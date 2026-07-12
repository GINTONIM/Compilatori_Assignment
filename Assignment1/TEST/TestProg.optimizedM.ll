; ModuleID = 'TestProg.m2r.ll'
source_filename = "TestProg.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @algebraic_identity(i32 noundef %0) #0 {
  %2 = add nsw i32 %0, 0
  %3 = mul nsw i32 %2, 1
  ret i32 %3
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @strength_reduction(i32 noundef %0) #0 {
  %2 = mul nsw i32 %0, 15
  %3 = sdiv i32 %2, 8
  ret i32 %3
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @multi_instruction(i32 noundef %0) #0 {
  %2 = sub nsw i32 %0, 55
  %3 = add nsw i32 %2, 55
  ret i32 %0
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 19.1.7 (++20250804090312+cd708029e0b2-1~exp1~20250804210325.79)"}
