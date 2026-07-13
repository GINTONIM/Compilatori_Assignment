; ModuleID = 'AdvancedLoop.ll'
source_filename = "AdvancedLoop.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [22 x i8] c"Risultato finale: %d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_licm_avanzato(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %16, %3
  %.01 = phi i32 [ 0, %3 ], [ %17, %16 ]
  %.0 = phi i32 [ 0, %3 ], [ %.1, %16 ]
  %5 = icmp slt i32 %.01, 100
  br i1 %5, label %6, label %18

6:                                                ; preds = %4
  %7 = add nsw i32 %0, %1
  %8 = add nsw i32 %7, %2
  %9 = srem i32 %.01, 2
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %11, label %14

11:                                               ; preds = %6
  %12 = mul nsw i32 %8, %0
  %13 = add nsw i32 %.0, %12
  br label %16

14:                                               ; preds = %6
  %15 = add nsw i32 %.0, %8
  br label %16

16:                                               ; preds = %14, %11
  %.1 = phi i32 [ %13, %11 ], [ %15, %14 ]
  %17 = add nsw i32 %.01, 1
  br label %4, !llvm.loop !6

18:                                               ; preds = %4
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
