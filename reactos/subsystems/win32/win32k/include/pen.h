#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include "gdiobj.h"
#include "brush.h"

/* Internal interface */

#define PENOBJ_AllocPen() ((HPEN)GDIOBJ_AllocObj(GDI_OBJECT_TYPE_PEN))
#define PENOBJ_FreePen(hBMObj) GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)
#define PENOBJ_LockPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))
#define PENOBJ_AllocExtPen() ((HPEN)GDIOBJ_AllocObj(GDI_OBJECT_TYPE_EXTPEN))
#define PENOBJ_FreeExtPen(hBMObj) GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN)
#define PENOBJ_LockExtPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN))
#define PENOBJ_UnlockPen(pPenObj) GDIOBJ_UnlockObjByPtr((POBJ)pPenObj)

INT STDCALL PEN_GetObject(PGDIBRUSHOBJ hPen, INT Count, PLOGPEN Buffer);

#endif
