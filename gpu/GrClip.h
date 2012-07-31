
/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#ifndef GrClip_DEFINED
#define GrClip_DEFINED

#include "GrClipIterator.h"
#include "GrRect.h"

#include "SkPath.h"
#include "SkTArray.h"

class GrSurface;

class GrClip {
public:
    GrClip();
    GrClip(const GrClip& src);
    GrClip(GrClipIterator* iter, const GrRect& conservativeBounds);
    explicit GrClip(const GrIRect& rect);
    explicit GrClip(const GrRect& rect);

    ~GrClip();

    GrClip& operator=(const GrClip& src);

    const GrRect& getConservativeBounds() const { 
        GrAssert(fConservativeBoundsValid);
        return fConservativeBounds; 
    }

    bool requiresAA() const { return fRequiresAA; }

    class Iter {
    public:
        enum IterStart {
            kBottom_IterStart,
            kTop_IterStart
        };

        /**
         * Creates an uninitialized iterator. Must be reset()
         */
        Iter();

        Iter(const GrClip& stack, IterStart startLoc);

        struct Clip {
            Clip() : fRect(NULL), fPath(NULL), fOp(SkRegion::kIntersect_Op), 
                     fDoAA(false) {}

            const SkRect*   fRect;  // if non-null, this is a rect clip
            const SkPath*   fPath;  // if non-null, this is a path clip
            SkRegion::Op    fOp;
            bool            fDoAA;
        };

        /**
         *  Return the clip for this element in the iterator. If next() returns
         *  NULL, then the iterator is done. The type of clip is determined by
         *  the pointers fRect and fPath:
         *
         *  fRect==NULL  fPath!=NULL    path clip
         *  fRect!=NULL  fPath==NULL    rect clip
         *  fRect==NULL  fPath==NULL    empty clip
         */
        const Clip* next();
        const Clip* prev();

        /**
         * Moves the iterator to the topmost clip with the specified RegionOp 
         * and returns that clip. If no clip with that op is found, 
         * returns NULL.
         */
        const Clip* skipToTopmost(SkRegion::Op op);

        /**
         * Restarts the iterator on a clip stack.
         */
        void reset(const GrClip& stack, IterStart startLoc);

    private:
        const GrClip*      fStack;
        Clip               fClip;
        int                fCurIndex;

        /**
         * updateClip updates fClip to represent the clip in the index slot of
         * GrClip's list. * It unifies functionality needed by both next() and
         * prev().
         */
        const Clip* updateClip(int index);
    };

private:
    int getElementCount() const { return fList.count(); }

    GrClipType getElementType(int i) const { return fList[i].fType; }

    const SkPath& getPath(int i) const {
        GrAssert(kPath_ClipType == fList[i].fType);
        return fList[i].fPath;
    }

    GrPathFill getPathFill(int i) const {
        GrAssert(kPath_ClipType == fList[i].fType);
        return fList[i].fPathFill;
    }

    const GrRect& getRect(int i) const {
        GrAssert(kRect_ClipType == fList[i].fType);
        return fList[i].fRect;
    }

    SkRegion::Op getOp(int i) const { return fList[i].fOp; }

    bool getDoAA(int i) const   { return fList[i].fDoAA; }

public:
    bool isRect() const {
        if (1 == fList.count() && kRect_ClipType == fList[0].fType && 
            (SkRegion::kIntersect_Op == fList[0].fOp ||
             SkRegion::kReplace_Op == fList[0].fOp)) {
            // if we determined that the clip is a single rect
            // we ought to have also used that rect as the bounds.
            GrAssert(fConservativeBoundsValid);
            GrAssert(fConservativeBounds == fList[0].fRect);
            return true;
        } else {
            return false;
        }
    }

    // isWideOpen returns true if the clip has no elements (it is the 
    // infinite plane) not that it has no area.
    bool isWideOpen() const { return 0 == fList.count(); }

    /**
     *  Resets this clip to be empty
     */
    void setEmpty();

    void setFromIterator(GrClipIterator* iter, const GrRect& conservativeBounds);
    void setFromRect(const GrRect& rect);
    void setFromIRect(const GrIRect& rect);

    friend bool operator==(const GrClip& a, const GrClip& b) {
        if (a.fList.count() != b.fList.count()) {
            return false;
        }
        int count = a.fList.count();
        for (int i = 0; i < count; ++i) {
            if (a.fList[i] != b.fList[i]) {
                return false;
            }
        }
        return true;
    }
    friend bool operator!=(const GrClip& a, const GrClip& b) {
        return !(a == b);
    }

private:
    struct Element {
        GrClipType   fType;
        GrRect       fRect;
        SkPath       fPath;
        GrPathFill   fPathFill;
        SkRegion::Op fOp;
        bool         fDoAA;
        bool operator ==(const Element& e) const {
            if (e.fType != fType || e.fOp != fOp || e.fDoAA != fDoAA) {
                return false;
            }
            switch (fType) {
                case kRect_ClipType:
                    return fRect == e.fRect;
                case kPath_ClipType:
                    return fPath == e.fPath;
                default:
                    GrCrash("Unknown clip element type.");
                    return false; // suppress warning
            }
        }
        bool operator !=(const Element& e) const { return !(*this == e); }
    };

    GrRect              fConservativeBounds;
    bool                fConservativeBoundsValid;

    bool                fRequiresAA;

    enum {
        kPreAllocElements = 4,
    };
    SkSTArray<kPreAllocElements, Element>   fList;
};

/**
 * GrClipData encapsulates the information required to construct the clip
 * masks. 'fOrigin' is only non-zero when saveLayer has been called
 * with an offset bounding box. The clips in 'fClipStack' are in 
 * device coordinates (i.e., they have been translated by -fOrigin w.r.t.
 * the canvas' device coordinates).
 */
class GrClipData : public SkNoncopyable {
public:
    const GrClip*       fClipStack;
    SkIPoint            fOrigin;

    GrClipData() 
        : fClipStack(NULL) {
        fOrigin.setZero();
    }

    bool operator==(const GrClipData& other) const {
        if (fOrigin != other.fOrigin) {
            return false;
        }

        if (NULL != fClipStack && NULL != other.fClipStack) {
            return *fClipStack == *other.fClipStack;
        }

        return fClipStack == other.fClipStack;
    }

    bool operator!=(const GrClipData& other) const { 
        return !(*this == other); 
    }

    void getConservativeBounds(const GrSurface* surface, 
                               GrIRect* devResult,
                               bool* isIntersectionOfRects = NULL) const;
};

#endif

