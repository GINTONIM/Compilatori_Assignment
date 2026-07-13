; ModuleID = 'FusionTest.m2r.ll'
source_filename = "FusioneTest.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @fusione_computazione(ptr noundef %0, ptr noundef %1, ptr noundef %2, ptr noundef %3) #0 {
  br label %5

5:                                                ; preds = %14, %4
  %.01 = phi i32 [ 0, %4 ], [ %15, %14 ]
  %6 = icmp slt i32 %.01, 100
  br i1 %6, label %7, label %16

7:                                                ; preds = %5
  %8 = sext i32 %.01 to i64
  %9 = getelementptr inbounds i32, ptr %1, i64 %8
  %10 = load i32, ptr %9, align 4
  %11 = mul nsw i32 %10, 2
  %12 = sext i32 %.01 to i64
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  store i32 %11, ptr %13, align 4
  br label %19

14:                                               ; preds = %19
  %15 = add nsw i32 %.01, 1
  br label %5, !llvm.loop !6

16:                                               ; preds = %5
  br label %17

17:                                               ; preds = %29, %16
  %.0 = phi i32 [ 0, %16 ], [ %30, %29 ]
  %18 = icmp slt i32 %.01, 100
  unreachable

19:                                               ; preds = %7
  %20 = sext i32 %.01 to i64
  %21 = getelementptr inbounds i32, ptr %0, i64 %20
  %22 = load i32, ptr %21, align 4
  %23 = sext i32 %.01 to i64
  %24 = getelementptr inbounds i32, ptr %3, i64 %23
  %25 = load i32, ptr %24, align 4
  %26 = add nsw i32 %22, %25
  %27 = sext i32 %.01 to i64
  %28 = getelementptr inbounds i32, ptr %2, i64 %27
  store i32 %26, ptr %28, align 4
  br label %14

29:                                               ; No predecessors!
  %30 = add nsw i32 %.01, 1
  br label %17, !llvm.loop !8

31:                                               ; No predecessors!
  ret void
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
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
