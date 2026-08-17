// ShadowHook uses this deliberately empty shared object to observe a real
// linker constructor event while discovering the platform soinfo layout.
// It must remain a separate ELF; linking this translation unit into the main
// module would remove the load event that drives the scan.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
static int a;
#pragma clang diagnostic pop
