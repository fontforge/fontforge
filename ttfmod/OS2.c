/* Copyright (C) 2001 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ttfmodui.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>


static unichar_t zero[] = { '0', '\0' };
static unichar_t one[] = { '1', '\0' };
static unichar_t two[] = { '2', '\0' };
static GTextInfo OS2versions[] = {
    { (unichar_t *) zero, NULL, 0, 0, (void *) 0},
    { (unichar_t *) one, NULL, 0, 0, (void *) 1},
    { (unichar_t *) two, NULL, 0, 0, (void *) 2},
    { NULL }};
static GTextInfo widthclass[] = {
    { (unichar_t *) _STR_UltraCondensed, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraCondensed, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condensed75, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SemiCondensed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium100, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SemiExpanded, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Expanded125, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraExpanded, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UltraExpanded, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo weightclass[] = {
    { (unichar_t *) _STR_Thin100, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraLight200, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Light300, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Book400, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium500, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DemiBold600, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bold700, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Heavy800, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Black900, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo fstype[] = {
    { (unichar_t *) _STR_NeverEmbeddable, NULL, 0, 0, (void *) 0x02, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OnlyPrint, NULL, 0, 0, (void *) 0x04, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EditableDoc, NULL, 0, 0, (void *) 0x0c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Installable, NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panfamily[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TextDisplay, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Script, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Decorative, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Pictoral, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panserifs[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Cove, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseCove, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SquareCove, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseSquareCove, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Square, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thin, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bone, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Exaggerated, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Triangle, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalSans, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseSans, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PerpSans, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Flared, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Rounded, NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panweight[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryLight, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Light, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thin, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Book, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Demi, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bold, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Heavy, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Black, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Nord, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panprop[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OldStyle, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Modern, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EvenWidth, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Expanded, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condensed, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryExpanded, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryCondensed, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Monospaced, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pancontrast[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_None, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryLow, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Low, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MediumLow, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MediumHigh, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_High, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryHigh, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panstrokevar[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradDiag, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradTrans, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradVert, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradHor, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_RapidVert, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_RapidHor, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstantVert, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panarmstyle[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsH, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsW, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsV, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsSS, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsDS, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsH, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsW, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsV, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsSS, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsDS, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panletterform[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalContact, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalWeighted, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalBoxed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalFlattened, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalRounded, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalOffCenter, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalSquare, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueContact, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueWeighted, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueBoxed, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueRounded, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueOffCenter, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueSquare, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panmidline[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardTrimmed, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardPointed, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardSerifed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighTrimmed, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighPointed, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighSerifed, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantTrimmed, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantPointed, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantSerifed, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowTrimmed, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowPointed, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowSerifed, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panxheight[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantSmall, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantStandard, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantLarge, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingSmall, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingStandard, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingLarge, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ibmfamily[] = {
    { (unichar_t *) _STR_NoClassification, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OldStyleSerifs, NULL, 0, 0, (void *) 0x100, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSRoundedLegibility, NULL, 0, 0, (void *) 0x101, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSGeralde, NULL, 0, 0, (void *) 0x102, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSVenetian, NULL, 0, 0, (void *) 0x103, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSModifiedVenetian, NULL, 0, 0, (void *) 0x104, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSDutchModern, NULL, 0, 0, (void *) 0x105, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSDutchTrad, NULL, 0, 0, (void *) 0x106, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSContemporary, NULL, 0, 0, (void *) 0x107, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSCaligraphic, NULL, 0, 0, (void *) 0x108, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OSSMiscellaneous, NULL, 0, 0, (void *) 0x10f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TransitionalSerifs, NULL, 0, 0, (void *) 0x200, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TSDirectLine, NULL, 0, 0, (void *) 0x201, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TSScript, NULL, 0, 0, (void *) 0x202, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TSMiscellaneous, NULL, 0, 0, (void *) 0x20f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ModernSerifs, NULL, 0, 0, (void *) 0x300, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MSItalian, NULL, 0, 0, (void *) 0x301, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MSScript, NULL, 0, 0, (void *) 0x302, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MSMiscellaneous, NULL, 0, 0, (void *) 0x30f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ClarendonSerifs, NULL, 0, 0, (void *) 0x400, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSClarendon, NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSModern, NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSTraditional, NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSNewspaper, NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSStubSerif, NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSMonotone, NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSTypewriter, NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CSMiscellaneous, NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SlabSerifs, NULL, 0, 0, (void *) 0x500, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSMonotone, NULL, 0, 0, (void *) 0x501, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSHumanist, NULL, 0, 0, (void *) 0x502, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSGeometric, NULL, 0, 0, (void *) 0x503, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSSwiss, NULL, 0, 0, (void *) 0x504, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSTypewriter, NULL, 0, 0, (void *) 0x505, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSMiscellaneous, NULL, 0, 0, (void *) 0x50f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FreeformSerifs, NULL, 0, 0, (void *) 0x700, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FSModern, NULL, 0, 0, (void *) 0x701, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FSMiscellaneous, NULL, 0, 0, (void *) 0x70f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SansSerif, NULL, 0, 0, (void *) 0x800, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSIBMNeoGrotesqueGothic, NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSHumanist, NULL, 0, 0, (void *) 0x802, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSLowxRoundGeometric, NULL, 0, 0, (void *) 0x803, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSHighxRoundGeometric, NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSNeoGrotesqueGothic, NULL, 0, 0, (void *) 0x805, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSModifiedGrotesqueGothic, NULL, 0, 0, (void *) 0x806, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSTypewriterGothic, NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSMatrix, NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SSMiscellaneous, NULL, 0, 0, (void *) 0x80f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Ornamentals, NULL, 0, 0, (void *) 0x900, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OEngraver, NULL, 0, 0, (void *) 0x901, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OBlackLetter, NULL, 0, 0, (void *) 0x902, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ODecorative, NULL, 0, 0, (void *) 0x903, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_O3D, NULL, 0, 0, (void *) 0x904, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OMiscellaneous, NULL, 0, 0, (void *) 0x90f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Scripts, NULL, 0, 0, (void *) 0xa00, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SUncial, NULL, 0, 0, (void *) 0xa01, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SBrushJoined, NULL, 0, 0, (void *) 0xa02, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SFormalJoined, NULL, 0, 0, (void *) 0xa03, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SMonotoneJoined, NULL, 0, 0, (void *) 0xa04, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SCaligraphic, NULL, 0, 0, (void *) 0xa05, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SBrushUnjoined, NULL, 0, 0, (void *) 0xa06, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SFormalUnjoined, NULL, 0, 0, (void *) 0xa07, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SMonotoneUnjoined, NULL, 0, 0, (void *) 0xa08, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SMiscellaneous, NULL, 0, 0, (void *) 0xa0f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Symbolic, NULL, 0, 0, (void *) 0xc00, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SyMixedSerif, NULL, 0, 0, (void *) 0xc03, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SyOldStyleSerif, NULL, 0, 0, (void *) 0xc06, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SyNeoGrotesqueSansSerif, NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SyMiscellaneous, NULL, 0, 0, (void *) 0xc0f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo unicoderangelist[] = {
    { (unichar_t *) _STR_BasicLatin, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Latin1Sup, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LatinExtA, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LatinExtB, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_IPAExten, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SpacingModLetters, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CombiningDiacritics, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UGreek, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GreekSymCoptic, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Cyrillic, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Armenian, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UHebrew, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HebrewExtended, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UArabic, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ArabicExtended, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Devanagari, NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bengali, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Gurmukhi, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Gujarati, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Oriya, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tamil, NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Telegu, NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Kannada, NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Malayalam, NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thai, NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Loa, NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Georgian, NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GeorgianExtended, NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HangulJamo, NULL, 0, 0, (void *) 28, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LatinAdditional, NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GreekAdditional, NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Punctuation, NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SubSuperscripts, NULL, 0, 0, (void *) 32, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Currency, NULL, 0, 0, (void *) 33, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CombSymbolDiac, NULL, 0, 0, (void *) 34, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LeterlikeSymbols, NULL, 0, 0, (void *) 35, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NumberForms, NULL, 0, 0, (void *) 36, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Arrows, NULL, 0, 0, (void *) 37, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MathOpers, NULL, 0, 0, (void *) 38, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MiscTech, NULL, 0, 0, (void *) 39, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ControlPictures, NULL, 0, 0, (void *) 40, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OCR, NULL, 0, 0, (void *) 41, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EnclosedAlphanumerics, NULL, 0, 0, (void *) 42, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BoxDrawing, NULL, 0, 0, (void *) 43, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BlockElements, NULL, 0, 0, (void *) 44, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GeometricShapes, NULL, 0, 0, (void *) 45, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MiscSymbols, NULL, 0, 0, (void *) 46, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Dingbats, NULL, 0, 0, (void *) 47, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJKSymPunct, NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hiragana, NULL, 0, 0, (void *) 49, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Katakana, NULL, 0, 0, (void *) 50, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bopomofo, NULL, 0, 0, (void *) 51, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HangulCompatJamo, NULL, 0, 0, (void *) 52, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJKMisc, NULL, 0, 0, (void *) 53, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EnclosedLettersMonths, NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJKCompat, NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hangul, NULL, 0, 0, (void *) 56, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Surrogates, NULL, 0, 0, (void *) 57, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unassignedbit58, NULL, 0, 0, (void *) 58, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJK, NULL, 0, 0, (void *) 59, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PrivateUse, NULL, 0, 0, (void *) 60, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJKCompatIdeo, NULL, 0, 0, (void *) 61, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_AlphabeticPresentationForms, NULL, 0, 0, (void *) 62, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ArabicPresentationFormsA, NULL, 0, 0, (void *) 63, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CombiningHalfMarks, NULL, 0, 0, (void *) 64, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CJKCompatForms, NULL, 0, 0, (void *) 65, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SmallForms, NULL, 0, 0, (void *) 66, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ArabicPresentationFormsB, NULL, 0, 0, (void *) 67, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HalfFullWidthForms, NULL, 0, 0, (void *) 68, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Specials, NULL, 0, 0, (void *) 69, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tibetan, NULL, 0, 0, (void *) 70, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Syriac, NULL, 0, 0, (void *) 71, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thaana, NULL, 0, 0, (void *) 72, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sinhala, NULL, 0, 0, (void *) 73, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Myanmar, NULL, 0, 0, (void *) 74, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Ethiopic, NULL, 0, 0, (void *) 75, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Cherokee, NULL, 0, 0, (void *) 76, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UnitedCanSyl, NULL, 0, 0, (void *) 77, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Ogham, NULL, 0, 0, (void *) 78, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Runic, NULL, 0, 0, (void *) 79, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Khmer, NULL, 0, 0, (void *) 80, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Mongolian, NULL, 0, 0, (void *) 81, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Braille, NULL, 0, 0, (void *) 82, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Yi, NULL, 0, 0, (void *) 83, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unassignedbit84, NULL, 0, 0, (void *) 84, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo fsselectionlist[] = {
    { (unichar_t *) _STR_Italic, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Underscore, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Negative, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Outlined, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Strikeout, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bold, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Regular, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo codepagelist[] = {
    { (unichar_t *) _STR_CPLatin1, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPLatin2, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPCyrillic, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPGreek, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPTurkish, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPHebrew, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPArabic, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPBaltic, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPVietnamese, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPThai, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPJapan, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPSimplifiedChinese, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPWansung, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPTraditionalChinese, NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPJohab, NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPMac, NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPOEM, NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPSymbol, NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPIBMGreek, NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPDOSRussian, NULL, 0, 0, (void *) 49, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPDOSNordic, NULL, 0, 0, (void *) 50, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPArabic2, NULL, 0, 0, (void *) 51, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPDOSFrench, NULL, 0, 0, (void *) 52, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPHebrew2, NULL, 0, 0, (void *) 53, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPDOSIcelandic, NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPDOSPortuguese, NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPIBMTurkish, NULL, 0, 0, (void *) 56, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPIBMCyrillic, NULL, 0, 0, (void *) 57, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPLatin22, NULL, 0, 0, (void *) 58, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPIBMBaltic, NULL, 0, 0, (void *) 59, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPGreek3, NULL, 0, 0, (void *) 60, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPArabic3, NULL, 0, 0, (void *) 61, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPLatin12, NULL, 0, 0, (void *) 62, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CPUS, NULL, 0, 0, (void *) 63, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

#define CID_Version		1000
#define CID_AvgWidth		1001
#define CID_WeightClass		1002
#define CID_WeightClassL	1003
#define CID_WidthClass		1004
#define CID_WidthClassL		1005
#define CID_FSType		1006
#define CID_FSTypeL		1007
#define CID_IBMFamily		1008
#define CID_IBMFamilyL		1009
#define CID_Vendor		1010
#define CID_Selection		1011
#define CID_SelectionAdd	1012
#define CID_SelectionRemove	1013
#define CID_SelectionShow	1014

#define CID_SubXSize		2001
#define CID_SubYSize		2002
#define CID_SubXOffset		2003
#define CID_SubYOffset		2004
#define CID_SupXSize		2005
#define CID_SupYSize		2006
#define CID_SupXOffset		2007
#define CID_SupYOffset		2008
#define CID_StrikeSize		2009
#define CID_StrikePos		2010

#define CID_PanFamily		4001
#define CID_PanSerifs		4002
#define CID_PanWeight		4003
#define CID_PanProp		4004
#define CID_PanContrast		4005
#define CID_PanStrokeVar	4006
#define CID_PanArmStyle		4007
#define CID_PanLetterform	4008
#define CID_PanMidLine		4009
#define CID_PanXHeight		4010

#define CID_UnicodeRanges	5001
#define CID_UnicodeAdd		5002
#define CID_UnicodeRemove	5003
#define CID_UnicodeShow		5004
#define CID_CodePageLab		5005
#define CID_CodePageRanges	5006
#define CID_CodePageAdd		5008
#define CID_CodePageRemove	5010
#define CID_CodePageShow	5011
#define CID_FirstUnicode	5012
#define CID_LastUnicode		5013

#define CID_TypoAscender	6001
#define CID_TypoDescender	6002
#define CID_TypoLineGap		6003
#define CID_WinAscent		6004
#define CID_WinDescent		6005
#define CID_XHeightLab		6106
#define CID_XHeight		6006
#define CID_CapHeightLab	6107
#define CID_CapHeight		6007
#define CID_DefCharLab		6108
#define CID_DefChar		6008
#define CID_DefCharS		6018
#define CID_BrkCharLab		6109
#define CID_BrkChar		6009
#define CID_BrkCharS		6019
#define CID_MaxContextLab	6110
#define CID_MaxContext		6010


typedef struct os2view /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
/* os2 specials */
    int16 old_aspect;
} OS2View;

static int os2_processdata(TableView *tv) {
    OS2View *os2v = (OS2View *) tv;
    /* Do changes!!! */
return( true );
}

static int os2_close(TableView *tv) {
    if ( os2_processdata(tv)) {
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs os2funcs = { os2_close, os2_processdata };

static int OS2_VersionChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret = _GGadgetGetTitle(g);
    int val = u_strtol(ret,NULL,10);

    if ( val==0 || val==1 || val==2 ) {
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CodePageLab),val!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CodePageRanges),val!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CodePageAdd),val!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CodePageRemove),val!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_XHeightLab),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_XHeight),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CapHeightLab),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_CapHeight),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_DefCharLab),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_DefChar),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_DefCharS),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_BrkCharLab),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_BrkChar),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_BrkCharS),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MaxContextLab),val==2);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MaxContext),val==2);
    }
return( true );
}

static int OS2_WeightChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    int val;
    char buffer[8]; unichar_t ub[8];

    if ( GGadgetGetCid(g)==CID_WeightClass ) {
	ret = _GGadgetGetTitle(g);
	val = u_strtol(ret,NULL,10);
	val = val/100 -1;
	if ( val<0 ) val = 0; else if ( val>8 ) val=8;
	GGadgetSelectOneListItem(GWidgetGetControl(gw,CID_WeightClassL),val);
    } else {
	val = GGadgetGetFirstListSelectedItem(g);
	val = 100*val + 100;
	sprintf(buffer,"%d", val);
	uc_strcpy(ub,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_WeightClass),ub);
    }
return( true );
}

static int OS2_WidthChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    int val;
    char buffer[8]; unichar_t ub[8];

    if ( GGadgetGetCid(g)==CID_WidthClass ) {
	ret = _GGadgetGetTitle(g);
	val = u_strtol(ret,NULL,10);
	--val;
	if ( val<0 ) val = 0; else if ( val>9 ) val=9;
	GGadgetSelectOneListItem(GWidgetGetControl(gw,CID_WidthClassL),val);
    } else {
	val = GGadgetGetFirstListSelectedItem(g);
	++val;
	sprintf(buffer,"%d", val);
	uc_strcpy(ub,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_WidthClass),ub);
    }
return( true );
}

static int OS2_TypeChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    int val;
    GTextInfo *sel;
    char buffer[8]; unichar_t ub[8];

    if ( GGadgetGetCid(g)==CID_FSType ) {
	ret = _GGadgetGetTitle(g);
	val = u_strtol(ret,NULL,16);
	if ( val&8 ) val = 2;
	else if ( val&4 ) val=1;
	else if ( val&2 ) val=0;
	else val = 3;
	GGadgetSelectOneListItem(GWidgetGetControl(gw,CID_FSTypeL),val);
    } else {
	sel = GGadgetGetListItemSelected(g);
	sprintf(buffer,"%x", (int) (sel->userdata));
	uc_strcpy(ub,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_FSType),ub);
    }
return( true );
}

static int OS2_FamilyChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    int val,i;
    char buffer[8]; unichar_t ub[8];

    if ( GGadgetGetCid(g)==CID_IBMFamily ) {
	ret = _GGadgetGetTitle(g);
	val = u_strtol(ret,NULL,10);
	for ( i=0; ibmfamily[i].text!=NULL; ++i )
	    if ( (int) (ibmfamily[i].userdata)==val )
	break;
	if ( ibmfamily[i].text!=NULL )
	    GGadgetSelectOneListItem(GWidgetGetControl(gw,CID_IBMFamilyL),i);
    } else {
	sprintf(buffer,"%04x", (int) (GGadgetGetListItemSelected(g)->userdata));
	uc_strcpy(ub,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_IBMFamily),ub);
    }
return( true );
}

static void readunicoderanges(GGadget *g,int32 flags[4]) {
    const unichar_t *ret;
    unichar_t *end;

    ret = _GGadgetGetTitle(g);
    flags[3] = u_strtoul(ret,&end,16);
    while ( !isdigit(*end) && *end!='\0' ) ++end;
    flags[2] = u_strtoul(end,&end,16);
    while ( !isdigit(*end) && *end!='\0' ) ++end;
    flags[1] = u_strtoul(end,&end,16);
    while ( !isdigit(*end) && *end!='\0' ) ++end;
    flags[0] = u_strtoul(end,&end,16);
}

static int _OS2_UnicodeChange(GGadget *g, int32 flags[4]) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    int len, size, i, set;
    GGadget *add, *remove, *show;
    GTextInfo **alist, **rlist;
    unichar_t *temp, *pt;

    add = GWidgetGetControl(gw,CID_UnicodeAdd);
    remove = GWidgetGetControl(gw,CID_UnicodeRemove);
    show = GWidgetGetControl(gw,CID_UnicodeShow);

    alist = GGadgetGetList(add,&len);
    rlist = GGadgetGetList(remove,&len);
    for ( i=size=0; i<len; ++i ) {
	set = (flags[i>>5]&(1<<(i&31)))?1 : 0;
	alist[i]->disabled =  set;
	rlist[i]->disabled = !set;
	alist[i]->selected = rlist[i]->selected = false;
	if ( set ) size += u_strlen(alist[i]->text)+2;
    }
    GGadgetSetTitle(add,GStringGetResource(_STR_Add,NULL));
    GGadgetSetTitle(remove,GStringGetResource(_STR_Remove,NULL));

    temp = pt = galloc((size+1)*sizeof(unichar_t));
    for ( i=0; i<len; ++i ) {
	if ( flags[i>>5]&(1<<(i&31)) ) {
	    u_strcpy(pt,alist[i]->text);
	    uc_strcat(pt,", ");
	    pt += u_strlen(pt);
	}
    }
    if ( size!=0 )
	pt-=2;
    *pt = '\0';
    GGadgetSetTitle(show,temp);
    free(temp);
    
return( true );
}

static int OS2_UnicodeChange(GGadget *g, GEvent *e) {
    int32 flags[4];

    if ( e!=NULL && (e->type!=et_controlevent || e->u.control.subtype != et_textchanged ))
return( true );

    readunicoderanges(g,flags);
return( _OS2_UnicodeChange(g,flags));
}

static int OS2_UnicodeChangeAddRemove(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    int32 flags[4];
    int bit;
    GGadget *field;
    char ranges[40]; unichar_t ur[40];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
return( true );
    field = GWidgetGetControl(gw,CID_UnicodeRanges);
    readunicoderanges(field,flags);
    bit = GGadgetGetFirstListSelectedItem(g);
    if ( GGadgetGetCid(g)==CID_UnicodeAdd )
	flags[bit>>5] |= (1<<(bit&31));
    else
	flags[bit>>5] &= ~(1<<(bit&31));
    _OS2_UnicodeChange(field,flags);
    sprintf( ranges, "%08x.%08x.%08x.%08x", flags[3], flags[2], flags[1], flags[0]);
    uc_strcpy(ur,ranges);
    GGadgetSetTitle(field,ur);
return( true );
}

static void readcodepages(GGadget *g,int32 flags[2]) {
    const unichar_t *ret;
    unichar_t *end;

    ret = _GGadgetGetTitle(g);
    flags[1] = u_strtoul(ret,&end,16);
    while ( !isdigit(*end) && *end!='\0' ) ++end;
    flags[0] = u_strtoul(end,&end,16);
}

static int _OS2_CodePageChange(GGadget *g, int32 flags[2]) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    int len, size, i, set, bit;
    GGadget *add, *remove, *show;
    GTextInfo **alist, **rlist;
    unichar_t *temp, *pt;

    add = GWidgetGetControl(gw,CID_CodePageAdd);
    remove = GWidgetGetControl(gw,CID_CodePageRemove);
    show = GWidgetGetControl(gw,CID_CodePageShow);

    alist = GGadgetGetList(add,&len);
    rlist = GGadgetGetList(remove,&len);
    for ( i=size=0; i<len; ++i ) {
	bit = (int) (alist[i]->userdata);
	set = (flags[bit>>5]&(1<<(bit&31)))?1 : 0;
	alist[i]->disabled =  set;
	rlist[i]->disabled = !set;
	alist[i]->selected = rlist[i]->selected = false;
	if ( set ) size += u_strlen(alist[i]->text)+2;
    }
    GGadgetSetTitle(add,GStringGetResource(_STR_Add,NULL));
    GGadgetSetTitle(remove,GStringGetResource(_STR_Remove,NULL));

    temp = pt = galloc((size+1)*sizeof(unichar_t));
    for ( i=0; i<len; ++i ) {
	bit = (int) (alist[i]->userdata);
	if ( flags[bit>>5]&(1<<(bit&31)) ) {
	    u_strcpy(pt,alist[i]->text);
	    uc_strcat(pt,", ");
	    pt += u_strlen(pt);
	}
    }
    if ( size!=0 )
	pt-=2;
    *pt = '\0';
    GGadgetSetTitle(show,temp);
    free(temp);
    
return( true );
}

static int OS2_CodePageChange(GGadget *g, GEvent *e) {
    int32 flags[2];

    if ( e!=NULL && (e->type!=et_controlevent || e->u.control.subtype != et_textchanged ))
return( true );

    readcodepages(g,flags);
return( _OS2_CodePageChange(g,flags));
}

static int OS2_CodePageChangeAddRemove(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    int32 flags[2];
    int bit;
    GGadget *field;
    char ranges[40]; unichar_t ur[40];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
return( true );
    field = GWidgetGetControl(gw,CID_CodePageRanges);
    readcodepages(field,flags);
    bit = (int) (GGadgetGetListItemSelected(g)->userdata);
    if ( GGadgetGetCid(g)==CID_CodePageAdd )
	flags[bit>>5] |= (1<<(bit&31));
    else
	flags[bit>>5] &= ~(1<<(bit&31));
    _OS2_CodePageChange(field,flags);
    sprintf( ranges, "%08x.%08x", flags[1], flags[0]);
    uc_strcpy(ur,ranges);
    GGadgetSetTitle(field,ur);
return( true );
}

static int _OS2_SelectionChange(GGadget *g, int32 flags) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    int len, size, i, set;
    GGadget *add, *remove, *show;
    GTextInfo **alist, **rlist;
    unichar_t *temp, *pt;

    add = GWidgetGetControl(gw,CID_SelectionAdd);
    remove = GWidgetGetControl(gw,CID_SelectionRemove);
    show = GWidgetGetControl(gw,CID_SelectionShow);

    alist = GGadgetGetList(add,&len);
    rlist = GGadgetGetList(remove,&len);
    for ( i=size=0; i<len; ++i ) {
	set = (flags&(1<<i))?1 : 0;
	alist[i]->disabled =  set;
	rlist[i]->disabled = !set;
	alist[i]->selected = rlist[i]->selected = false;
	if ( set ) size += u_strlen(alist[i]->text)+2;
    }
    GGadgetSetTitle(add,GStringGetResource(_STR_Add,NULL));
    GGadgetSetTitle(remove,GStringGetResource(_STR_Remove,NULL));

    temp = pt = galloc((size+1)*sizeof(unichar_t));
    for ( i=0; i<len; ++i ) {
	if ( flags&(1<<i) ) {
	    u_strcpy(pt,alist[i]->text);
	    uc_strcat(pt,", ");
	    pt += u_strlen(pt);
	}
    }
    if ( size!=0 )
	pt-=2;
    *pt = '\0';
    GGadgetSetTitle(show,temp);
    free(temp);
    
return( true );
}

static int OS2_SelectionChange(GGadget *g, GEvent *e) {
    int32 flags;
    const unichar_t *ret;

    if ( e!=NULL && (e->type!=et_controlevent || e->u.control.subtype != et_textchanged ))
return( true );

    ret = GGadgetGetTitle(g);
    flags = u_strtoul(ret,NULL,16);
return( _OS2_SelectionChange(g,flags));
}

static int OS2_SelectionChangeAddRemove(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    int32 flags;
    int bit;
    GGadget *field;
    char sel[40]; unichar_t us[40];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
return( true );
    field = GWidgetGetControl(gw,CID_Selection);
    ret = GGadgetGetTitle(field);
    flags = u_strtoul(ret,NULL,16);
    bit = GGadgetGetFirstListSelectedItem(g);
    if ( GGadgetGetCid(g)==CID_SelectionAdd ) {
	flags |= (1<<bit);
	if ( bit==6 ) flags = 1<<6;		/* If regular is set, all others must be clear */
	else flags &= ~(1<<6);			/* If something else is set, regular must be clear */
    } else
	flags &= ~(1<<bit);
    _OS2_SelectionChange(field,flags);
    sprintf( sel, "%04x", flags);
    uc_strcpy(us,sel);
    GGadgetSetTitle(field,us);
return( true );
}

static int OS2_DefBrkChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret;
    unichar_t newchar[2];
    GGadget *other;

    if ( e->type!=et_controlevent || e->u.control.subtype != et_textchanged )
return( true );
    other = GWidgetGetControl(gw,GGadgetGetCid(g)==CID_DefChar?CID_DefCharS
							      :CID_BrkCharS);
    ret = GGadgetGetTitle(g);
    newchar[0] = u_strtoul(ret,NULL,16);
    newchar[1] = 0;
    GGadgetSetTitle(other,newchar);
return( true );
}

static int OS2_AspectChange(GGadget *g, GEvent *e) {
return( true );
}

static int OS2_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    OS2View *os2v;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	os2v = GDrawGetUserData(gw);
	os2_close((TableView *) os2v);
    }
return( true );
}

static int OS2_OK(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int os2_e_h(GWindow gw, GEvent *event) {
    OS2View *os2v = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	GDrawDestroyWindow(os2v->gw);
    } else if ( event->type == et_destroy ) {
	os2v->table->tv = NULL;
	free(os2v);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(os2v->table->name);
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    }
return( true );
}

/* OpenType adds:
    short sxHeight;		/ * xHeight * /
    short scapHeight;
    ushort usDefaultChar;
    ushort usBreakChar;
    ushort usMaxContext;
*/
void OS2CreateEditor(Table *tab,TtfView *tfv) {
    OS2View *os2v = gcalloc(1,sizeof(OS2View));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[8];
    GGadgetCreateData mgcd[6], ggcd[24], pangcd[22], agcd[24], cgcd[24], tgcd[24];
    GTextInfo mlabel[6], glabel[24], panlabel[22], alabel[24], clabel[24], tlabel[25];
    static unichar_t title[60] = { 'O', 'S', '/', '2', ' ',  '\0' };
    char version[8], avgwidth[8], weight[8], width[8], type[8], aligns[9][8],
	family[8], vendor[8], ranges[40], first[8], last[8], sel[8],
	codepages[40], asndr[8], dsndr[8], asnt[8], dsnt[8], linegap[8], xh[8],
	ch[8], dc[8], bc[8], mcontext[8];
    unichar_t _dc[2], _bc[2];
    int vnum, temp, i,j;
    static int anames[] = { _STR_SubXSize, _STR_SubYSize, _STR_SubXOffset, _STR_SubYOffset,
	    _STR_SupXSize, _STR_SupYSize, _STR_SupXOffset, _STR_SupYOffset,
	    _STR_StrikeSize, _STR_StrikePos, 0 };

    os2v->table = tab;
    os2v->virtuals = &os2funcs;
    os2v->owner = tfv;
    os2v->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) os2v;

    TableFillup(tab);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    u_strncpy(title+5, os2v->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,312);
    pos.height = GDrawPointsToPixels(NULL,340);
    os2v->gw = gw = GDrawCreateTopWindow(NULL,&pos,os2_e_h,os2v,&wattrs);

/******************************************************************************/
    memset(&glabel,0,sizeof(glabel));
    memset(&ggcd,0,sizeof(ggcd));

    ggcd[0].gd.pos.x = 10; ggcd[0].gd.pos.y = 12;
    glabel[0].text = (unichar_t *) _STR_Version;
    glabel[0].text_in_resource = true;
    ggcd[0].gd.label = &glabel[0];
    ggcd[0].gd.flags = gg_visible | gg_enabled;
    ggcd[0].creator = GLabelCreate;

    vnum = tgetushort(tab,0);
    sprintf( version, "%d", vnum );
    glabel[1].text = (unichar_t *) version;
    glabel[1].text_is_1byte = true;
    ggcd[1].gd.label = &glabel[1];
    ggcd[1].gd.pos.x = 80; ggcd[1].gd.pos.y = ggcd[0].gd.pos.y-6;
    ggcd[1].gd.flags = gg_enabled|gg_visible;
    ggcd[1].gd.cid = CID_Version;
    ggcd[1].gd.u.list = OS2versions;
    ggcd[1].gd.handle_controlevent = OS2_VersionChange;
    ggcd[1].creator = GListFieldCreate;

    ggcd[2].gd.pos.x = 10; ggcd[2].gd.pos.y = ggcd[1].gd.pos.y+24+6;
    glabel[2].text = (unichar_t *) _STR_AvgWidth;
    glabel[2].text_in_resource = true;
    ggcd[2].gd.label = &glabel[2];
    ggcd[2].gd.flags = gg_visible | gg_enabled;
    ggcd[2].creator = GLabelCreate;

    sprintf( avgwidth, "%d", tgetushort(tab,2) );
    glabel[3].text = (unichar_t *) avgwidth;
    glabel[3].text_is_1byte = true;
    ggcd[3].gd.label = &glabel[3];
    ggcd[3].gd.pos.x = 80; ggcd[3].gd.pos.y = ggcd[2].gd.pos.y-6;
    ggcd[3].gd.flags = gg_enabled|gg_visible;
    ggcd[3].gd.cid = CID_AvgWidth;
    ggcd[3].creator = GTextFieldCreate;

    ggcd[4].gd.pos.x = 10; ggcd[4].gd.pos.y = ggcd[3].gd.pos.y+24+6;
    glabel[4].text = (unichar_t *) _STR_WeightClass;
    glabel[4].text_in_resource = true;
    ggcd[4].gd.label = &glabel[4];
    ggcd[4].gd.flags = gg_visible | gg_enabled;
    ggcd[4].creator = GLabelCreate;

    sprintf( weight, "%d", tgetushort(tab,4) );
    glabel[5].text = (unichar_t *) weight;
    glabel[5].text_is_1byte = true;
    ggcd[5].gd.label = &glabel[5];
    ggcd[5].gd.pos.x = 80; ggcd[5].gd.pos.y = ggcd[4].gd.pos.y-6; ggcd[5].gd.pos.width = 60;
    ggcd[5].gd.flags = gg_enabled|gg_visible;
    ggcd[5].gd.cid = CID_WeightClass;
    ggcd[5].gd.handle_controlevent = OS2_WeightChange;
    ggcd[5].creator = GTextFieldCreate;

    ggcd[6].gd.pos.x = 150; ggcd[6].gd.pos.y = ggcd[5].gd.pos.y;
    ggcd[6].gd.flags = gg_visible | gg_enabled;
    ggcd[6].gd.cid = CID_WeightClassL;
    ggcd[6].gd.u.list = weightclass;
    ggcd[6].gd.handle_controlevent = OS2_WeightChange;
    ggcd[6].creator = GListButtonCreate;
    temp = (tgetushort(tab,4)+50)/100 - 1;
    if ( temp<0 ) temp = 0; else if ( temp>8 ) temp=8;
    for ( i=0; weightclass[i].text!=NULL; ++i )
	weightclass[i].selected = i==temp;

    ggcd[7].gd.pos.x = 10; ggcd[7].gd.pos.y = ggcd[5].gd.pos.y+24+6;
    glabel[7].text = (unichar_t *) _STR_WidthClass;
    glabel[7].text_in_resource = true;
    ggcd[7].gd.label = &glabel[7];
    ggcd[7].gd.flags = gg_visible | gg_enabled;
    ggcd[7].creator = GLabelCreate;

    sprintf( width, "%d", temp=tgetushort(tab,6) );
    glabel[8].text = (unichar_t *) width;
    glabel[8].text_is_1byte = true;
    ggcd[8].gd.label = &glabel[8];
    ggcd[8].gd.pos.x = 80; ggcd[8].gd.pos.y = ggcd[7].gd.pos.y-6; ggcd[8].gd.pos.width = 60;
    ggcd[8].gd.flags = gg_enabled|gg_visible;
    ggcd[8].gd.cid = CID_WidthClass;
    ggcd[8].gd.handle_controlevent = OS2_WidthChange;
    ggcd[8].creator = GTextFieldCreate;

    ggcd[9].gd.pos.x = 150; ggcd[9].gd.pos.y = ggcd[8].gd.pos.y; ggcd[9].gd.pos.width = 140;
    ggcd[9].gd.flags = gg_visible | gg_enabled;
    ggcd[9].gd.cid = CID_WidthClassL;
    ggcd[9].gd.u.list = widthclass;
    ggcd[9].gd.handle_controlevent = OS2_WidthChange;
    ggcd[9].creator = GListButtonCreate;
    if ( temp<1 ) temp = 1; else if ( temp>9 ) temp=9;
    for ( i=0; widthclass[i].text!=NULL; ++i )
	widthclass[i].selected = i==temp-1;

    ggcd[10].gd.pos.x = 10; ggcd[10].gd.pos.y = ggcd[8].gd.pos.y+24+6;
    glabel[10].text = (unichar_t *) _STR_Embeddable;
    glabel[10].text_in_resource = true;
    ggcd[10].gd.label = &glabel[10];
    ggcd[10].gd.flags = gg_visible | gg_enabled;
    ggcd[10].gd.popup_msg = GStringGetResource(_STR_EmbeddablePopup,NULL);
    ggcd[10].creator = GLabelCreate;

    sprintf( type, "%x", temp=tgetushort(tab,8) );
    glabel[11].text = (unichar_t *) type;
    glabel[11].text_is_1byte = true;
    ggcd[11].gd.label = &glabel[11];
    ggcd[11].gd.pos.x = 80; ggcd[11].gd.pos.y = ggcd[10].gd.pos.y-6; ggcd[11].gd.pos.width = 60;
    ggcd[11].gd.flags = gg_enabled|gg_visible;
    ggcd[11].gd.cid = CID_FSType;
    ggcd[11].gd.handle_controlevent = OS2_TypeChange;
    ggcd[11].creator = GTextFieldCreate;

    ggcd[12].gd.pos.x = 150; ggcd[12].gd.pos.y = ggcd[11].gd.pos.y; ggcd[12].gd.pos.width = ggcd[9].gd.pos.width;
    ggcd[12].gd.flags = gg_visible | gg_enabled;
    ggcd[12].gd.cid = CID_FSTypeL;
    ggcd[12].gd.u.list = fstype;
    ggcd[12].gd.handle_controlevent = OS2_TypeChange;
    ggcd[12].creator = GListButtonCreate;
    for ( i=0; fstype[i].text!=NULL; ++i )
	fstype[i].selected = false;
    if ( temp&8 ) fstype[2].selected = true;
    else if ( temp&4 ) fstype[1].selected = true;
    else if ( temp&2 ) fstype[0].selected = true;
    else fstype[3].selected = true;

    ggcd[13].gd.pos.x = 10; ggcd[13].gd.pos.y = ggcd[11].gd.pos.y+24+6;
    glabel[13].text = (unichar_t *) _STR_IBMFamily;
    glabel[13].text_in_resource = true;
    ggcd[13].gd.label = &glabel[13];
    ggcd[13].gd.flags = gg_visible | gg_enabled;
    ggcd[13].creator = GLabelCreate;

    sprintf( family, "%04x", temp=tgetushort(tab,30) );
    glabel[14].text = (unichar_t *) family;
    glabel[14].text_is_1byte = true;
    ggcd[14].gd.label = &glabel[14];
    ggcd[14].gd.pos.x = 80; ggcd[14].gd.pos.y = ggcd[13].gd.pos.y-6; ggcd[14].gd.pos.width = 60;
    ggcd[14].gd.flags = gg_enabled|gg_visible;
    ggcd[14].gd.cid = CID_IBMFamily;
    ggcd[14].gd.handle_controlevent = OS2_FamilyChange;
    ggcd[14].creator = GTextFieldCreate;

    ggcd[15].gd.pos.x = 150; ggcd[15].gd.pos.y = ggcd[14].gd.pos.y; ggcd[15].gd.pos.width = ggcd[9].gd.pos.width;
    ggcd[15].gd.flags = gg_visible | gg_enabled;
    ggcd[15].gd.cid = CID_IBMFamilyL;
    ggcd[15].gd.u.list = ibmfamily;
    ggcd[15].gd.handle_controlevent = OS2_FamilyChange;
    ggcd[15].creator = GListButtonCreate;
    for ( i=0; ibmfamily[i].text!=NULL; ++i )
	ibmfamily[i].selected = temp==(int)ibmfamily[i].userdata;

    ggcd[16].gd.pos.x = 10; ggcd[16].gd.pos.y = ggcd[14].gd.pos.y+24+6;
    glabel[16].text = (unichar_t *) _STR_VendorID;
    glabel[16].text_in_resource = true;
    ggcd[16].gd.label = &glabel[16];
    ggcd[16].gd.flags = gg_visible | gg_enabled;
    ggcd[16].creator = GLabelCreate;

    vendor[0] = tab->data[58]; vendor[1] = tab->data[59]; vendor[2] = tab->data[60]; vendor[3] = tab->data[61];
    vendor[4] = 0;
    glabel[17].text = (unichar_t *) vendor;
    glabel[17].text_is_1byte = true;
    ggcd[17].gd.label = &glabel[17];
    ggcd[17].gd.pos.x = 80; ggcd[17].gd.pos.y = ggcd[16].gd.pos.y-6; ggcd[17].gd.pos.width = 60;
    ggcd[17].gd.flags = gg_enabled|gg_visible;
    ggcd[17].gd.cid = CID_Vendor;
    ggcd[17].creator = GTextFieldCreate;

    ggcd[18].gd.pos.x = 10; ggcd[18].gd.pos.y = ggcd[17].gd.pos.y+24+6;
    glabel[18].text = (unichar_t *) _STR_Selection;
    glabel[18].text_in_resource = true;
    ggcd[18].gd.label = &glabel[18];
    ggcd[18].gd.flags = gg_visible | gg_enabled;
    ggcd[18].creator = GLabelCreate;

    sprintf( sel, "%04x", tgetushort(tab,62) );
    glabel[19].text = (unichar_t *) sel;
    glabel[19].text_is_1byte = true;
    ggcd[19].gd.label = &glabel[19];
    ggcd[19].gd.pos.x = 80; ggcd[19].gd.pos.y = ggcd[18].gd.pos.y-6; ggcd[19].gd.pos.width = 60;
    ggcd[19].gd.flags = gg_enabled|gg_visible;
    ggcd[19].gd.cid = CID_Selection;
    ggcd[19].gd.handle_controlevent = OS2_SelectionChange;
    ggcd[19].creator = GTextFieldCreate;

    ggcd[20].gd.pos.x = 150; ggcd[20].gd.pos.y = ggcd[19].gd.pos.y; ggcd[20].gd.pos.width = 60;
    ggcd[20].gd.flags = gg_visible | gg_enabled;
    ggcd[20].gd.cid = CID_SelectionAdd;
    ggcd[20].gd.u.list = fsselectionlist;
    glabel[20].text = (unichar_t *) _STR_Add;
    glabel[20].text_in_resource = true;
    ggcd[20].gd.label = &glabel[20];
    ggcd[20].gd.handle_controlevent = OS2_SelectionChangeAddRemove;
    ggcd[20].creator = GListButtonCreate;

    ggcd[21].gd.pos.x = 220; ggcd[21].gd.pos.y = ggcd[20].gd.pos.y; ggcd[21].gd.pos.width = 60;
    ggcd[21].gd.flags = gg_visible | gg_enabled;
    ggcd[21].gd.cid = CID_SelectionRemove;
    ggcd[21].gd.u.list = fsselectionlist;
    glabel[21].text = (unichar_t *) _STR_Remove;
    glabel[21].text_in_resource = true;
    ggcd[21].gd.label = &glabel[21];
    ggcd[21].gd.handle_controlevent = OS2_SelectionChangeAddRemove;
    ggcd[21].creator = GListButtonCreate;

    ggcd[22].gd.pos.x = 10; ggcd[22].gd.pos.y = ggcd[20].gd.pos.y+26; ggcd[22].gd.pos.width = 270;
    ggcd[22].gd.flags = gg_visible;
    ggcd[22].gd.cid = CID_SelectionShow;
    ggcd[22].creator = GTextFieldCreate;

/******************************************************************************/
    memset(&alabel,0,sizeof(alabel));
    memset(&agcd,0,sizeof(agcd));

    for ( i=0; i<20; i +=2 ) {
	agcd[i].gd.pos.x = 10; agcd[i].gd.pos.y = i==0?12: agcd[i-1].gd.pos.y+24+6;
	alabel[i].text = (unichar_t *) anames[i/2];
	alabel[i].text_in_resource = true;
	agcd[i].gd.label = &alabel[i];
	agcd[i].gd.flags = gg_visible | gg_enabled;
	agcd[i].creator = GLabelCreate;

	sprintf( aligns[i/2], "%d", (short) tgetushort(tab,10+i) );
	alabel[i+1].text = (unichar_t *) aligns[i/2];
	alabel[i+1].text_is_1byte = true;
	agcd[i+1].gd.label = &alabel[i+1];
	agcd[i+1].gd.pos.x = 80; agcd[i+1].gd.pos.y = agcd[i].gd.pos.y-6;
	agcd[i+1].gd.flags = gg_enabled|gg_visible;
	agcd[i+1].gd.cid = CID_SubXSize + i/2;
	agcd[i+1].creator = GTextFieldCreate;
    }

/******************************************************************************/
    memset(&panlabel,0,sizeof(panlabel));
    memset(&pangcd,0,sizeof(pangcd));

    pangcd[0].gd.pos.x = 10; pangcd[0].gd.pos.y = 12;
    panlabel[0].text = (unichar_t *) _STR_Family;
    panlabel[0].text_in_resource = true;
    pangcd[0].gd.label = &panlabel[0];
    pangcd[0].gd.flags = gg_visible | gg_enabled;
    pangcd[0].creator = GLabelCreate;

    pangcd[1].gd.pos.x = 100; pangcd[1].gd.pos.y = pangcd[0].gd.pos.y-6; pangcd[1].gd.pos.width = 120;
    pangcd[1].gd.flags = gg_visible | gg_enabled;
    pangcd[1].gd.cid = CID_PanFamily;
    pangcd[1].gd.u.list = panfamily;
    pangcd[1].creator = GListButtonCreate;

    pangcd[2].gd.pos.x = 10; pangcd[2].gd.pos.y = pangcd[1].gd.pos.y+24+6;
    panlabel[2].text = (unichar_t *) _STR_Serifs;
    panlabel[2].text_in_resource = true;
    pangcd[2].gd.label = &panlabel[2];
    pangcd[2].gd.flags = gg_visible | gg_enabled;
    pangcd[2].creator = GLabelCreate;

    pangcd[3].gd.pos.x = 100; pangcd[3].gd.pos.y = pangcd[2].gd.pos.y-6; pangcd[3].gd.pos.width = 120;
    pangcd[3].gd.flags = gg_visible | gg_enabled;
    pangcd[3].gd.cid = CID_PanSerifs;
    pangcd[3].gd.u.list = panserifs;
    pangcd[3].creator = GListButtonCreate;

    pangcd[4].gd.pos.x = 10; pangcd[4].gd.pos.y = pangcd[3].gd.pos.y+24+6;
    panlabel[4].text = (unichar_t *) _STR_Weight;
    panlabel[4].text_in_resource = true;
    pangcd[4].gd.label = &panlabel[4];
    pangcd[4].gd.flags = gg_visible | gg_enabled;
    pangcd[4].creator = GLabelCreate;

    pangcd[5].gd.pos.x = 100; pangcd[5].gd.pos.y = pangcd[4].gd.pos.y-6; pangcd[5].gd.pos.width = 120;
    pangcd[5].gd.flags = gg_visible | gg_enabled;
    pangcd[5].gd.cid = CID_PanWeight;
    pangcd[5].gd.u.list = panweight;
    pangcd[5].creator = GListButtonCreate;

    pangcd[6].gd.pos.x = 10; pangcd[6].gd.pos.y = pangcd[5].gd.pos.y+24+6;
    panlabel[6].text = (unichar_t *) _STR_Proportion;
    panlabel[6].text_in_resource = true;
    pangcd[6].gd.label = &panlabel[6];
    pangcd[6].gd.flags = gg_visible | gg_enabled;
    pangcd[6].creator = GLabelCreate;

    pangcd[7].gd.pos.x = 100; pangcd[7].gd.pos.y = pangcd[6].gd.pos.y-6; pangcd[7].gd.pos.width = 120;
    pangcd[7].gd.flags = gg_visible | gg_enabled;
    pangcd[7].gd.cid = CID_PanProp;
    pangcd[7].gd.u.list = panprop;
    pangcd[7].creator = GListButtonCreate;

    pangcd[8].gd.pos.x = 10; pangcd[8].gd.pos.y = pangcd[7].gd.pos.y+24+6;
    panlabel[8].text = (unichar_t *) _STR_Contrast;
    panlabel[8].text_in_resource = true;
    pangcd[8].gd.label = &panlabel[8];
    pangcd[8].gd.flags = gg_visible | gg_enabled;
    pangcd[8].creator = GLabelCreate;

    pangcd[9].gd.pos.x = 100; pangcd[9].gd.pos.y = pangcd[8].gd.pos.y-6; pangcd[9].gd.pos.width = 120;
    pangcd[9].gd.flags = gg_visible | gg_enabled;
    pangcd[9].gd.cid = CID_PanContrast;
    pangcd[9].gd.u.list = pancontrast;
    pangcd[9].creator = GListButtonCreate;

    pangcd[10].gd.pos.x = 10; pangcd[10].gd.pos.y = pangcd[9].gd.pos.y+24+6;
    panlabel[10].text = (unichar_t *) _STR_StrokeVar;
    panlabel[10].text_in_resource = true;
    pangcd[10].gd.label = &panlabel[10];
    pangcd[10].gd.flags = gg_visible | gg_enabled;
    pangcd[10].creator = GLabelCreate;

    pangcd[11].gd.pos.x = 100; pangcd[11].gd.pos.y = pangcd[10].gd.pos.y-6; pangcd[11].gd.pos.width = 120;
    pangcd[11].gd.flags = gg_visible | gg_enabled;
    pangcd[11].gd.cid = CID_PanStrokeVar;
    pangcd[11].gd.u.list = panstrokevar;
    pangcd[11].creator = GListButtonCreate;

    pangcd[12].gd.pos.x = 10; pangcd[12].gd.pos.y = pangcd[11].gd.pos.y+24+6;
    panlabel[12].text = (unichar_t *) _STR_ArmStyle;
    panlabel[12].text_in_resource = true;
    pangcd[12].gd.label = &panlabel[12];
    pangcd[12].gd.flags = gg_visible | gg_enabled;
    pangcd[12].creator = GLabelCreate;

    pangcd[13].gd.pos.x = 100; pangcd[13].gd.pos.y = pangcd[12].gd.pos.y-6; pangcd[13].gd.pos.width = 120;
    pangcd[13].gd.flags = gg_visible | gg_enabled;
    pangcd[13].gd.cid = CID_PanArmStyle;
    pangcd[13].gd.u.list = panarmstyle;
    pangcd[13].creator = GListButtonCreate;

    pangcd[14].gd.pos.x = 10; pangcd[14].gd.pos.y = pangcd[13].gd.pos.y+24+6;
    panlabel[14].text = (unichar_t *) _STR_Letterform;
    panlabel[14].text_in_resource = true;
    pangcd[14].gd.label = &panlabel[14];
    pangcd[14].gd.flags = gg_visible | gg_enabled;
    pangcd[14].creator = GLabelCreate;

    pangcd[15].gd.pos.x = 100; pangcd[15].gd.pos.y = pangcd[14].gd.pos.y-6; pangcd[15].gd.pos.width = 120;
    pangcd[15].gd.flags = gg_visible | gg_enabled;
    pangcd[15].gd.cid = CID_PanLetterform;
    pangcd[15].gd.u.list = panletterform;
    pangcd[15].creator = GListButtonCreate;

    pangcd[16].gd.pos.x = 10; pangcd[16].gd.pos.y = pangcd[15].gd.pos.y+24+6;
    panlabel[16].text = (unichar_t *) _STR_MidLine;
    panlabel[16].text_in_resource = true;
    pangcd[16].gd.label = &panlabel[16];
    pangcd[16].gd.flags = gg_visible | gg_enabled;
    pangcd[16].creator = GLabelCreate;

    pangcd[17].gd.pos.x = 100; pangcd[17].gd.pos.y = pangcd[16].gd.pos.y-6; pangcd[17].gd.pos.width = 120;
    pangcd[17].gd.flags = gg_visible | gg_enabled;
    pangcd[17].gd.cid = CID_PanMidLine;
    pangcd[17].gd.u.list = panmidline;
    pangcd[17].creator = GListButtonCreate;

    pangcd[18].gd.pos.x = 10; pangcd[18].gd.pos.y = pangcd[17].gd.pos.y+24+6;
    panlabel[18].text = (unichar_t *) _STR_XHeight;
    panlabel[18].text_in_resource = true;
    pangcd[18].gd.label = &panlabel[18];
    pangcd[18].gd.flags = gg_visible | gg_enabled;
    pangcd[18].creator = GLabelCreate;

    pangcd[19].gd.pos.x = 100; pangcd[19].gd.pos.y = pangcd[18].gd.pos.y-6; pangcd[19].gd.pos.width = 120;
    pangcd[19].gd.flags = gg_visible | gg_enabled;
    pangcd[19].gd.cid = CID_PanXHeight;
    pangcd[19].gd.u.list = panxheight;
    pangcd[19].creator = GListButtonCreate;

    for ( i=1; i<=19; i+=2 ) {
	for ( j=0; pangcd[i].gd.u.list[j].text!=NULL; ++j )
	    pangcd[i].gd.u.list[j].selected = false;
	pangcd[i].gd.u.list[tab->data[32+(i-1)/2]].selected = true;
    }
/******************************************************************************/
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));

    cgcd[0].gd.pos.x = 10; cgcd[0].gd.pos.y = 6;
    clabel[0].text = (unichar_t *) _STR_UnicodeRanges;
    clabel[0].text_in_resource = true;
    cgcd[0].gd.label = &clabel[0];
    cgcd[0].gd.flags = gg_visible | gg_enabled;
    cgcd[0].creator = GLabelCreate;

    sprintf( ranges, "%08x.%08x.%08x.%08x", tgetlong(tab,54), tgetlong(tab,50), tgetlong(tab,46), tgetlong(tab,42) );
    clabel[1].text = (unichar_t *) ranges;
    clabel[1].text_is_1byte = true;
    cgcd[1].gd.label = &clabel[1];
    cgcd[1].gd.pos.x = 10; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13; cgcd[1].gd.pos.width = 270;
    cgcd[1].gd.flags = gg_visible | gg_enabled;
    cgcd[1].gd.cid = CID_UnicodeRanges;
    cgcd[1].gd.handle_controlevent = OS2_UnicodeChange;
    cgcd[1].creator = GTextFieldCreate;

    cgcd[2].gd.pos.x = 20; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+24; cgcd[2].gd.pos.width = 120;
    cgcd[2].gd.flags = gg_visible | gg_enabled;
    cgcd[2].gd.cid = CID_UnicodeAdd;
    cgcd[2].gd.u.list = unicoderangelist;
    clabel[2].text = (unichar_t *) _STR_Add;
    clabel[2].text_in_resource = true;
    cgcd[2].gd.label = &clabel[2];
    cgcd[2].gd.handle_controlevent = OS2_UnicodeChangeAddRemove;
    cgcd[2].creator = GListButtonCreate;

    cgcd[3].gd.pos.x = 160; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y; cgcd[3].gd.pos.width = 120;
    cgcd[3].gd.flags = gg_visible | gg_enabled;
    cgcd[3].gd.cid = CID_UnicodeRemove;
    cgcd[3].gd.u.list = unicoderangelist;
    clabel[3].text = (unichar_t *) _STR_Remove;
    clabel[3].text_in_resource = true;
    cgcd[3].gd.label = &clabel[3];
    cgcd[3].gd.handle_controlevent = OS2_UnicodeChangeAddRemove;
    cgcd[3].creator = GListButtonCreate;

    cgcd[4].gd.pos.x = 10; cgcd[4].gd.pos.y = cgcd[2].gd.pos.y+26; cgcd[4].gd.pos.width = 270;
    cgcd[4].gd.flags = gg_visible | gg_textarea_wrap;
    cgcd[4].gd.cid = CID_UnicodeShow;
    cgcd[4].creator = GTextAreaCreate;

    cgcd[5].gd.pos.x = 10; cgcd[5].gd.pos.y = cgcd[4].gd.pos.y+57+6;
    clabel[5].text = (unichar_t *) _STR_FirstUnicode;
    clabel[5].text_in_resource = true;
    cgcd[5].gd.label = &clabel[5];
    cgcd[5].gd.flags = gg_visible | gg_enabled;
    cgcd[5].creator = GLabelCreate;

    sprintf( first, "U+%04x", tgetushort(tab,64));
    clabel[6].text = (unichar_t *) first;
    clabel[6].text_is_1byte = true;
    cgcd[6].gd.label = &clabel[6];
    cgcd[6].gd.pos.x = 80; cgcd[6].gd.pos.y = cgcd[5].gd.pos.y-6; cgcd[6].gd.pos.width = 60;
    cgcd[6].gd.flags = gg_visible | gg_enabled;
    cgcd[6].gd.cid = CID_FirstUnicode;
    cgcd[6].creator = GTextFieldCreate;

    cgcd[7].gd.pos.x = 155; cgcd[7].gd.pos.y = cgcd[5].gd.pos.y;
    clabel[7].text = (unichar_t *) _STR_LastUnicode;
    clabel[7].text_in_resource = true;
    cgcd[7].gd.label = &clabel[7];
    cgcd[7].gd.flags = gg_visible | gg_enabled;
    cgcd[7].creator = GLabelCreate;

    sprintf( last, "U+%04x", tgetushort(tab,66));
    clabel[8].text = (unichar_t *) last;
    clabel[8].text_is_1byte = true;
    cgcd[8].gd.label = &clabel[8];
    cgcd[8].gd.pos.x = 190; cgcd[8].gd.pos.y = cgcd[6].gd.pos.y; cgcd[8].gd.pos.width = 60;
    cgcd[8].gd.flags = gg_visible | gg_enabled;
    cgcd[8].gd.cid = CID_LastUnicode;
    cgcd[8].creator = GTextFieldCreate;

    cgcd[9].gd.pos.x = 10; cgcd[9].gd.pos.y = cgcd[8].gd.pos.y+24;
    clabel[9].text = (unichar_t *) _STR_CodePages;
    clabel[9].text_in_resource = true;
    cgcd[9].gd.label = &clabel[9];
    cgcd[9].gd.flags = gg_visible | gg_enabled;
    cgcd[9].gd.cid = CID_CodePageLab;
    cgcd[9].creator = GLabelCreate;

    sprintf( codepages, "%08x.%08x", tgetlong(tab,82), tgetlong(tab,78));
    clabel[10].text = (unichar_t *) codepages;
    clabel[10].text_is_1byte = true;
    cgcd[10].gd.label = &clabel[10];
    cgcd[10].gd.pos.x = 10; cgcd[10].gd.pos.y = cgcd[9].gd.pos.y+13; cgcd[10].gd.pos.width = 270;
    cgcd[10].gd.flags = gg_visible | gg_enabled;
    cgcd[10].gd.cid = CID_CodePageRanges;
    cgcd[10].gd.handle_controlevent = OS2_CodePageChange;
    cgcd[10].creator = GTextFieldCreate;

    cgcd[11].gd.pos.x = 20; cgcd[11].gd.pos.y = cgcd[10].gd.pos.y+24; cgcd[11].gd.pos.width = 120;
    cgcd[11].gd.flags = gg_visible | gg_enabled;
    cgcd[11].gd.cid = CID_CodePageAdd;
    cgcd[11].gd.u.list = codepagelist;
    clabel[11].text = (unichar_t *) _STR_Add;
    clabel[11].text_in_resource = true;
    cgcd[11].gd.label = &clabel[11];
    cgcd[11].gd.handle_controlevent = OS2_CodePageChangeAddRemove;
    cgcd[11].creator = GListButtonCreate;

    cgcd[12].gd.pos.x = 160; cgcd[12].gd.pos.y = cgcd[11].gd.pos.y; cgcd[12].gd.pos.width = 120;
    cgcd[12].gd.flags = gg_visible | gg_enabled;
    cgcd[12].gd.cid = CID_CodePageRemove;
    cgcd[12].gd.u.list = codepagelist;
    clabel[12].text = (unichar_t *) _STR_Remove;
    clabel[12].text_in_resource = true;
    cgcd[12].gd.label = &clabel[12];
    cgcd[12].gd.handle_controlevent = OS2_CodePageChangeAddRemove;
    cgcd[12].creator = GListButtonCreate;

    cgcd[13].gd.pos.x = 10; cgcd[13].gd.pos.y = cgcd[11].gd.pos.y+26; cgcd[13].gd.pos.width = 270;
    cgcd[13].gd.flags = gg_visible | gg_textarea_wrap;
    cgcd[13].gd.cid = CID_CodePageShow;
    cgcd[13].creator = GTextAreaCreate;

    if ( vnum==0 ) {
	codepages[0] = '\0';
	cgcd[9].gd.flags &= ~gg_enabled;
	cgcd[10].gd.flags &= ~gg_enabled;
	cgcd[11].gd.flags &= ~gg_enabled;
	cgcd[12].gd.flags &= ~gg_enabled;
    }
/******************************************************************************/
    memset(&tlabel,0,sizeof(tlabel));
    memset(&tgcd,0,sizeof(tgcd));

    tgcd[0].gd.pos.x = 10; tgcd[0].gd.pos.y = 12;
    tlabel[0].text = (unichar_t *) _STR_TypoAscender;
    tlabel[0].text_in_resource = true;
    tgcd[0].gd.label = &tlabel[0];
    tgcd[0].gd.flags = gg_visible | gg_enabled;
    tgcd[0].creator = GLabelCreate;

    sprintf( asndr, "%d", (short) tgetushort(tab,68) );
    tlabel[1].text = (unichar_t *) asndr;
    tlabel[1].text_is_1byte = true;
    tgcd[1].gd.label = &tlabel[1];
    tgcd[1].gd.pos.x = 80; tgcd[1].gd.pos.y = tgcd[0].gd.pos.y-6; tgcd[1].gd.pos.width = 60;
    tgcd[1].gd.flags = gg_enabled|gg_visible;
    tgcd[1].gd.cid = CID_TypoAscender;
    tgcd[1].creator = GTextFieldCreate;

    tgcd[2].gd.pos.x = 10; tgcd[2].gd.pos.y = tgcd[0].gd.pos.y+24;
    tlabel[2].text = (unichar_t *) _STR_TypoDescender;
    tlabel[2].text_in_resource = true;
    tgcd[2].gd.label = &tlabel[2];
    tgcd[2].gd.flags = gg_visible | gg_enabled;
    tgcd[2].creator = GLabelCreate;

    sprintf( dsndr, "%d", (short) tgetushort(tab,70) );
    tlabel[3].text = (unichar_t *) dsndr;
    tlabel[3].text_is_1byte = true;
    tgcd[3].gd.label = &tlabel[3];
    tgcd[3].gd.pos.x = tgcd[1].gd.pos.x; tgcd[3].gd.pos.y = tgcd[2].gd.pos.y-6; tgcd[3].gd.pos.width = 60;
    tgcd[3].gd.flags = gg_enabled|gg_visible;
    tgcd[3].gd.cid = CID_TypoDescender;
    tgcd[3].creator = GTextFieldCreate;

    tgcd[4].gd.pos.x = 10; tgcd[4].gd.pos.y = tgcd[2].gd.pos.y+24;
    tlabel[4].text = (unichar_t *) _STR_TypoLineGap;
    tlabel[4].text_in_resource = true;
    tgcd[4].gd.label = &tlabel[4];
    tgcd[4].gd.flags = gg_visible | gg_enabled;
    tgcd[4].creator = GLabelCreate;

    sprintf( linegap, "%d", (short) tgetushort(tab,72) );
    tlabel[5].text = (unichar_t *) linegap;
    tlabel[5].text_is_1byte = true;
    tgcd[5].gd.label = &tlabel[5];
    tgcd[5].gd.pos.x = tgcd[1].gd.pos.x; tgcd[5].gd.pos.y = tgcd[4].gd.pos.y-6; tgcd[5].gd.pos.width = 60;
    tgcd[5].gd.flags = gg_enabled|gg_visible;
    tgcd[5].gd.cid = CID_TypoLineGap;
    tgcd[5].creator = GTextFieldCreate;

    tgcd[6].gd.pos.x = 150; tgcd[6].gd.pos.y = tgcd[0].gd.pos.y;
    tlabel[6].text = (unichar_t *) _STR_WinAscent;
    tlabel[6].text_in_resource = true;
    tgcd[6].gd.label = &tlabel[6];
    tgcd[6].gd.flags = gg_visible | gg_enabled;
    tgcd[6].creator = GLabelCreate;

    sprintf( asnt, "%d", (short) tgetushort(tab,74) );
    tlabel[7].text = (unichar_t *) asnt;
    tlabel[7].text_is_1byte = true;
    tgcd[7].gd.label = &tlabel[7];
    tgcd[7].gd.pos.x = 213; tgcd[7].gd.pos.y = tgcd[6].gd.pos.y-6; tgcd[7].gd.pos.width = 60;
    tgcd[7].gd.flags = gg_enabled|gg_visible;
    tgcd[7].gd.cid = CID_WinAscent;
    tgcd[7].creator = GTextFieldCreate;

    tgcd[8].gd.pos.x = tgcd[6].gd.pos.x; tgcd[8].gd.pos.y = tgcd[2].gd.pos.y;
    tlabel[8].text = (unichar_t *) _STR_WinDescent;
    tlabel[8].text_in_resource = true;
    tgcd[8].gd.label = &tlabel[8];
    tgcd[8].gd.flags = gg_visible | gg_enabled;
    tgcd[8].creator = GLabelCreate;

    sprintf( dsnt, "%d", (short) tgetushort(tab,76) );
    tlabel[9].text = (unichar_t *) dsnt;
    tlabel[9].text_is_1byte = true;
    tgcd[9].gd.label = &tlabel[9];
    tgcd[9].gd.pos.x = tgcd[7].gd.pos.x; tgcd[9].gd.pos.y = tgcd[8].gd.pos.y-6; tgcd[9].gd.pos.width = 60;
    tgcd[9].gd.flags = gg_enabled|gg_visible;
    tgcd[9].gd.cid = CID_WinDescent;
    tgcd[9].creator = GTextFieldCreate;

    tgcd[10].gd.pos.x = 10; tgcd[10].gd.pos.y = tgcd[4].gd.pos.y+24;
    tlabel[10].text = (unichar_t *) _STR_XHeightC;
    tlabel[10].text_in_resource = true;
    tgcd[10].gd.label = &tlabel[10];
    tgcd[10].gd.flags = gg_visible | gg_enabled;
    tgcd[10].gd.cid = CID_XHeightLab;
    tgcd[10].creator = GLabelCreate;

    sprintf( xh, "%d", (short) tgetushort(tab,86) );
    tlabel[11].text = (unichar_t *) xh;
    tlabel[11].text_is_1byte = true;
    tgcd[11].gd.label = &tlabel[11];
    tgcd[11].gd.pos.x = tgcd[1].gd.pos.x; tgcd[11].gd.pos.y = tgcd[10].gd.pos.y-6; tgcd[11].gd.pos.width = 60;
    tgcd[11].gd.flags = gg_enabled|gg_visible;
    tgcd[11].gd.cid = CID_XHeight;
    tgcd[11].creator = GTextFieldCreate;

    tgcd[12].gd.pos.x = tgcd[6].gd.pos.x; tgcd[12].gd.pos.y = tgcd[10].gd.pos.y;
    tlabel[12].text = (unichar_t *) _STR_CapHeightC;
    tlabel[12].text_in_resource = true;
    tgcd[12].gd.label = &tlabel[12];
    tgcd[12].gd.flags = gg_visible | gg_enabled;
    tgcd[12].gd.cid = CID_CapHeightLab;
    tgcd[12].creator = GLabelCreate;

    sprintf( ch, "%d", (short) tgetushort(tab,88) );
    tlabel[13].text = (unichar_t *) ch;
    tlabel[13].text_is_1byte = true;
    tgcd[13].gd.label = &tlabel[13];
    tgcd[13].gd.pos.x = tgcd[7].gd.pos.x; tgcd[13].gd.pos.y = tgcd[12].gd.pos.y-6; tgcd[13].gd.pos.width = 60;
    tgcd[13].gd.flags = gg_enabled|gg_visible;
    tgcd[13].gd.cid = CID_CapHeight;
    tgcd[13].creator = GTextFieldCreate;

    tgcd[14].gd.pos.x = 10; tgcd[14].gd.pos.y = tgcd[10].gd.pos.y+24;
    tlabel[14].text = (unichar_t *) _STR_DefaultChar;
    tlabel[14].text_in_resource = true;
    tgcd[14].gd.label = &tlabel[14];
    tgcd[14].gd.flags = gg_visible | gg_enabled;
    tgcd[14].gd.cid = CID_DefCharLab;
    tgcd[14].creator = GLabelCreate;

    sprintf( dc, "%d", _dc[0] = tgetushort(tab,90) );
    tlabel[15].text = (unichar_t *) dc;
    tlabel[15].text_is_1byte = true;
    tgcd[15].gd.label = &tlabel[15];
    tgcd[15].gd.pos.x = tgcd[1].gd.pos.x; tgcd[15].gd.pos.y = tgcd[14].gd.pos.y-6; tgcd[15].gd.pos.width = 60;
    tgcd[15].gd.flags = gg_enabled|gg_visible;
    tgcd[15].gd.cid = CID_DefChar;
    tgcd[15].gd.handle_controlevent = OS2_DefBrkChange;
    tgcd[15].creator = GTextFieldCreate;

    tgcd[16].gd.pos.x = tgcd[15].gd.pos.x+tgcd[15].gd.pos.width+10;
    tgcd[16].gd.pos.y = tgcd[14].gd.pos.y; tgcd[16].gd.pos.width = 10;
    _dc[1] = 0;
    tlabel[16].text = _dc;
    tgcd[16].gd.label = &tlabel[16];
    tgcd[16].gd.flags = gg_visible | gg_enabled;
    tgcd[16].gd.cid = CID_DefCharS;
    tgcd[16].creator = GLabelCreate;

    tgcd[17].gd.pos.x = 10; tgcd[17].gd.pos.y = tgcd[14].gd.pos.y+24;
    tlabel[17].text = (unichar_t *) _STR_BreakChar;
    tlabel[17].text_in_resource = true;
    tgcd[17].gd.label = &tlabel[17];
    tgcd[17].gd.flags = gg_visible | gg_enabled;
    tgcd[17].gd.cid = CID_BrkCharLab;
    tgcd[17].creator = GLabelCreate;

    sprintf( bc, "%d", _bc[0] = tgetushort(tab,92) );
    tlabel[18].text = (unichar_t *) bc;
    tlabel[18].text_is_1byte = true;
    tgcd[18].gd.label = &tlabel[18];
    tgcd[18].gd.pos.x = tgcd[1].gd.pos.x; tgcd[18].gd.pos.y = tgcd[17].gd.pos.y-6; tgcd[18].gd.pos.width = 60;
    tgcd[18].gd.flags = gg_enabled|gg_visible;
    tgcd[18].gd.cid = CID_BrkChar;
    tgcd[18].gd.handle_controlevent = OS2_DefBrkChange;
    tgcd[18].creator = GTextFieldCreate;

    tgcd[19].gd.pos.x = tgcd[18].gd.pos.x+tgcd[18].gd.pos.width+10;
    tgcd[19].gd.pos.y = tgcd[17].gd.pos.y; tgcd[19].gd.pos.width = 10;
    _bc[1] = 0;
    tlabel[19].text = _bc;
    tgcd[19].gd.label = &tlabel[19];
    tgcd[19].gd.flags = gg_visible | gg_enabled;
    tgcd[19].gd.cid = CID_BrkCharS;
    tgcd[19].creator = GLabelCreate;

    tgcd[20].gd.pos.x = 10; tgcd[20].gd.pos.y = tgcd[17].gd.pos.y+24;
    tlabel[20].text = (unichar_t *) _STR_MaxContext;
    tlabel[20].text_in_resource = true;
    tgcd[20].gd.label = &tlabel[20];
    tgcd[20].gd.flags = gg_visible | gg_enabled;
    tgcd[20].gd.cid = CID_MaxContextLab;
    tgcd[20].creator = GLabelCreate;

    sprintf( mcontext, "%d", tgetushort(tab,94) );
    tlabel[21].text = (unichar_t *) mcontext;
    tlabel[21].text_is_1byte = true;
    tgcd[21].gd.label = &tlabel[21];
    tgcd[21].gd.pos.x = tgcd[1].gd.pos.x; tgcd[21].gd.pos.y = tgcd[20].gd.pos.y-6; tgcd[21].gd.pos.width = 60;
    tgcd[21].gd.flags = gg_enabled|gg_visible;
    tgcd[21].gd.cid = CID_MaxContext;
    tgcd[21].creator = GTextFieldCreate;

    if ( vnum==0 || vnum==1 ) {
	xh[0] = ch[0] = mcontext[0] = bc[0] = dc[0] = _bc[0] = _dc[0] = '\0';
	tgcd[10].gd.flags &= ~gg_enabled;
	tgcd[11].gd.flags &= ~gg_enabled;
	tgcd[12].gd.flags &= ~gg_enabled;
	tgcd[13].gd.flags &= ~gg_enabled;
	tgcd[14].gd.flags &= ~gg_enabled;
	tgcd[15].gd.flags &= ~gg_enabled;
	tgcd[16].gd.flags &= ~gg_enabled;
	tgcd[17].gd.flags &= ~gg_enabled;
	tgcd[18].gd.flags &= ~gg_enabled;
	tgcd[19].gd.flags &= ~gg_enabled;
	tgcd[20].gd.flags &= ~gg_enabled;
	tgcd[21].gd.flags &= ~gg_enabled;
    }
/******************************************************************************/


    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _STR_General;
    aspects[i].selected = true;
    os2v->old_aspect = 0;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = ggcd;

    aspects[i].text = (unichar_t *) _STR_Alignment;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = agcd;

    aspects[i].text = (unichar_t *) _STR_Panose;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = pangcd;

    aspects[i].text = (unichar_t *) _STR_Charsets;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = cgcd;

    aspects[i].text = (unichar_t *) _STR_Typography;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = tgcd;

    mgcd[0].gd.pos.x = 5; mgcd[0].gd.pos.y = 5;
    mgcd[0].gd.pos.width = 302;
    mgcd[0].gd.pos.height = 292;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.handle_controlevent = OS2_AspectChange;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+8-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _STR_OK;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = OS2_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = 312-GIntGetResource(_NUM_Buttonsize)-30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _STR_Cancel;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = OS2_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,mgcd);

    OS2_UnicodeChange(cgcd[1].ret,NULL);		/* Initialize unicoderanges */
    OS2_CodePageChange(cgcd[10].ret,NULL);		/* Initialize codepages */
    OS2_SelectionChange(ggcd[19].ret,NULL);		/* Initialize fsselection */

    GDrawSetVisible(gw,true);
}
