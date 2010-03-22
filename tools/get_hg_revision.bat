@echo #ifndef OC_HG_REVISION
@for /f "usebackq delims=" %%I in (`hg id --id`) do @echo #define OC_HG_REVISION "%%I"
@echo #endif

