// Temporary: DYLD_INSERT_LIBRARIES shim that makes CGColorSpaceCreateWithICCData() hand back a colour space
// that nothing but the caller keeps alive, as macOS 27.2 does. Older releases return an instance that
// CoreGraphics' own cache also retains, which hides Qt 6.11.1's early release of it (QTBUG-147602).
#include <CoreGraphics/CoreGraphics.h>
#include <malloc/malloc.h>
#include <stdio.h>
#include <stdlib.h>

typedef CGColorSpaceRef (*Factory)(CFDataRef iccData);

static CGColorSpaceRef calibrated(CFDataRef iccData)
{
    (void)iccData;
    const CGFloat whitePoint[3] = {0.9505, 1.0, 1.089};
    const CGFloat gamma[3] = {1.8, 1.8, 1.8};
    return CGColorSpaceCreateCalibratedRGB(whitePoint, NULL, gamma, NULL);
}

static CGColorSpaceRef iccBased(CFDataRef iccData)
{
    const CGFloat range[6] = {0, 1, 0, 1, 0, 1};
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(iccData);
    CGColorSpaceRef alternate = CGColorSpaceCreateDeviceRGB();
    CGColorSpaceRef result = CGColorSpaceCreateICCBased(3, range, provider, alternate);
    CGColorSpaceRelease(alternate);
    CGDataProviderRelease(provider);
    return result;
}

static Factory chosen = NULL;
static int calls = 0;

static int freedOnLastRelease(Factory factory, CFDataRef sample, const char* name)
{
    CGColorSpaceRef probe = factory(sample);
    if (!probe) {
        fprintf(stderr, "uncached_colorspace: %s returned NULL\n", name);
        return 0;
    }
    const void* address = probe;
    const long retainCount = CFGetRetainCount(probe);
    CGColorSpaceRelease(probe);
    const size_t sizeAfterRelease = malloc_size(address);
    fprintf(stderr, "uncached_colorspace: %s retain count %ld, malloc_size after release %zu\n", name, retainCount, sizeAfterRelease);
    return retainCount == 1 && sizeAfterRelease == 0;
}

static CGColorSpaceRef replacementCreateWithICCData(CFTypeRef data)
{
    CGColorSpaceRef original = CGColorSpaceCreateWithICCData(data);
    if (!original || !chosen) {
        return original;
    }
    CGColorSpaceRef fresh = chosen((CFDataRef)data);
    if (!fresh) {
        return original;
    }
    CGColorSpaceRelease(original);
    if (++calls <= 3) {
        fprintf(stderr, "uncached_colorspace: CGColorSpaceCreateWithICCData -> fresh %p (retain count %ld)\n", (const void*)fresh, CFGetRetainCount(fresh));
    }
    return fresh;
}

__attribute__((constructor)) static void pickFactory(void)
{
    CGColorSpaceRef srgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CFDataRef sample = CGColorSpaceCopyICCData(srgb);
    CGColorSpaceRelease(srgb);
    if (freedOnLastRelease(calibrated, sample, "CGColorSpaceCreateCalibratedRGB")) {
        chosen = calibrated;
    } else if (freedOnLastRelease(iccBased, sample, "CGColorSpaceCreateICCBased")) {
        chosen = iccBased;
    }
    CFRelease(sample);
    fprintf(stderr, "uncached_colorspace: %s\n", chosen ? "active" : "NO UNCACHED FACTORY FOUND - shim inactive");
}

__attribute__((used, section("__DATA,__interpose"))) static const struct {
    const void* replacement;
    const void* replacee;
} interposers[] = {{(const void*)replacementCreateWithICCData, (const void*)CGColorSpaceCreateWithICCData}};
