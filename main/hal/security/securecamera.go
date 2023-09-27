package mtksecurecamera

import (
	"android/soong/android"
	"android/soong/cc"
)

func mtkSCamTestDefaults(ctx android.LoadHookContext) {
	type props struct {
		Cppflags    []string
		Shared_libs []string
	}
	p := &props{}
	vars := ctx.Config().VendorConfig("mtkPlugin")
	if vars.Bool("MTK_ENABLE_GENIEZONE") {
		p.Shared_libs = append(p.Shared_libs, "libgz_uree")
		p.Cppflags = append(p.Cppflags, "-DMTK_MTEE_SUPPORTED=true")
	} else {
		p.Cppflags = append(p.Cppflags, "-DMTK_MTEE_SUPPORTED=false")
	}
	if vars.Bool("MTK_CAM_GENIEZONE_SUPPORT") {
		p.Cppflags = append(p.Cppflags, "-DMTKCAM_MTEE_PATH_SUPPORTED=true")
	} else {
		p.Cppflags = append(p.Cppflags, "-DMTKCAM_MTEE_PATH_SUPPORTED=false")
	}
	ctx.AppendProperties(p)
}

func init() {
	android.RegisterModuleType("mtk_libsecurecameratest_legacy_defaults", mtkSCamTestDefaultsFactory)
}

func mtkSCamTestDefaultsFactory() android.Module {
	module := cc.DefaultsFactory()
	android.AddLoadHook(module, mtkSCamTestDefaults)
	return module
}
