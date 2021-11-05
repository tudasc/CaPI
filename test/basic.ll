; ModuleID = 'basic.cpp'
source_filename = "basic.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i32 @_Z3fooi(i32 %0) #0 {
  %2 = tail call i8* @llvm.returnaddress(i32 0)
  tail call void @__cyg_profile_func_enter(i8* bitcast (i32 (i32)* @_Z3fooi to i8*), i8* %2) #3
  %3 = mul nsw i32 %0, %0
  tail call void @__cyg_profile_func_exit(i8* bitcast (i32 (i32)* @_Z3fooi to i8*), i8* %2) #3
  ret i32 %3
}

; Function Attrs: norecurse nounwind uwtable
define dso_local i32 @main(i32 %0, i8** nocapture readnone %1) local_unnamed_addr #1 {
  %3 = tail call i8* @llvm.returnaddress(i32 0) #3
  tail call void @__cyg_profile_func_enter(i8* bitcast (i32 (i32)* @_Z3fooi to i8*), i8* %3) #3
  %4 = mul nsw i32 %0, %0
  tail call void @__cyg_profile_func_exit(i8* bitcast (i32 (i32)* @_Z3fooi to i8*), i8* %3) #3
  ret i32 %4
}

declare void @__cyg_profile_func_enter(i8*, i8*) local_unnamed_addr

; Function Attrs: nounwind readnone
declare i8* @llvm.returnaddress(i32 immarg) #2

declare void @__cyg_profile_func_exit(i8*, i8*) local_unnamed_addr

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 (git@github.com:llvm/llvm-project.git ef32c611aa214dea855364efd7ba451ec5ec3f74)"}
