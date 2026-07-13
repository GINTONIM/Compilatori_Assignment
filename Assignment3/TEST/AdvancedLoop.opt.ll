; ModuleID = 'AdvancedLoop.m2r.ll'
source_filename = "AdvancedLoop.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [22 x i8] c"Risultato finale: %d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_licm_avanzato(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = add nsw i32 %0, %1
  %5 = add nsw i32 %4, %2
  %6 = mul nsw i32 %5, %0
  br label %7

7:                                                ; preds = %16, %3
  %.01 = phi i32 [ 0, %3 ], [ %17, %16 ]
  %.0 = phi i32 [ 0, %3 ], [ %.1, %16 ]
  %8 = icmp slt i32 %.01, 100
  br i1 %8, label %9, label %18

9:                                                ; preds = %7
  %10 = srem i32 %.01, 2
  %11 = icmp eq i32 %10, 0
  br i1 %11, label %12, label %14

12:                                               ; preds = %9
  %13 = add nsw i32 %.0, %6
  br label %16

14:                                               ; preds = %9
  %15 = add nsw i32 %.0, %5
  br label %16

16:                                               ; preds = %14, %12
  %.1 = phi i32 [ %13, %12 ], [ %15, %14 ]
  %17 = add nsw i32 %.01, 1
  br label %7, !llvm.loop !6

18:                                               ; preds = %7
  %19 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %.0)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  call void @test_licm_avanzato(i32 noundef 5, i32 noundef 10, i32 noundef 15)
  ret i32 0
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 19.1.7 (++20250804090312+cd708029e0b2-1~exp1~20250804210325.79)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
