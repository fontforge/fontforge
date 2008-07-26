#include <stdlib.h>
#include <string.h>
#include </Developer/Headers/FlatCarbon/Files.h>
#include </Developer/Headers/FlatCarbon/CarbonEvents.h>

/* This marvelous bit of code is designed as a bridge between the mac world */
/*  and the unix world. It nestles in the heart of the Mac FontForge application*/
/*  whence it waits for apple events. It then repackages these events in a way*/
/*  that makes sense for the unix fontforge program. The effect is that it */
/*  looks as though the unix program is responding to apple events -- */
/* So the unix program will open files, quit, etc. in response to the finder's*/
/*  requests. */
/* Quit might not work. It'll do ok if we get signaled before X, but if X dies*/
/*  first we're out of luck */

/* Or that's the hope */


/* Here's the pathspec to the unix program which brings up the windows under X*/
/*  The "-unique" is a magic argument that tells ff to check if there's another*/
/*  ff running on the display, and if so, pass the arguments on to it, and exit*/
#define FONTFORGE	PREFIX "/bin/fontforge -unique "


/* These are the four apple events to which we currently respond */
static pascal OSErr OpenApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    system( FONTFORGE " -home -open &" );
return( noErr );
}

static pascal OSErr ReopenApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    system( FONTFORGE " -home &" );
return( noErr );
}

static pascal OSErr OpenDocumentsAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    AEDescList  docList;
    FSRef       theFSRef;
    long        index;
    long        count = 0;
    OSErr       err;
    char	buffer[2048];
    char	*sofar = NULL;

    err = AEGetParamDesc(theAppleEvent, keyDirectObject,
                         typeAEList, &docList);
    err = AECountItems(&docList, &count);
    for(index = 1; index <= count; index++) {
        err = AEGetNthPtr(&docList, index, typeFSRef,
                        NULL, NULL, &theFSRef,
                        sizeof(theFSRef), NULL);// 4
	err = FSRefMakePath(&theFSRef,(unsigned char *) buffer,sizeof(buffer));
	if ( sofar==NULL )
	    sofar = strdup(buffer);
	else {
	    char *temp = realloc(sofar,strlen(sofar)+strlen(buffer)+10);
	    strcat(temp," ");
	    strcat(temp,buffer);
	    sofar = temp;
	}
    }
    AEDisposeDesc(&docList);

    if ( sofar!=NULL ) {
	char *ff = FONTFORGE;
	char *cmd = malloc(strlen(ff) + strlen(sofar) + 3 );
	strcpy(cmd,ff);
	strcat(cmd,sofar);
	strcat(cmd," &");
	system( cmd );
	free(sofar);
	free(cmd);
    }
return( err );
}

static pascal OSErr QuitApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    system( FONTFORGE "-quit &" );
return( errAEEventNotHandled );
}

/* Install event handlers for the Apple Events we care about */
static  OSErr   InstallMacOSEventHandlers(void) {
    OSErr       err;

    err     = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                NewAEEventHandlerUPP(OpenApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEReopenApplication,
                NewAEEventHandlerUPP(ReopenApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                NewAEEventHandlerUPP(OpenDocumentsAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                NewAEEventHandlerUPP(QuitApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

CantInstallAppleEventHandler:
    return err;

}

/* Sit there eating events until doomsday */
int main(int argc, char **argv) {

    InstallMacOSEventHandlers();
    RunApplicationEventLoop();
return( 0 );
}


/*
Musings on how this might be done inside fontforge itself...

OSStatus ReceiveNextEvent( UInt32 inNumTypes, const EventTypeSpec *inList,
	EventTimeout inTimeout, Boolean inPullEvent, EventRef *outEvent);
<Time out is in seconds, so it isn't very useful.
RecieveNextEvent(0,NULL,???,true,&event);

    EventRef theEvent;
    EventTargetRef theTarget;

    theTarget = GetEventDispatcherTarget();
    while  (ReceiveNextEvent(0, NULL,kEventDurationForever,true,
            &theEvent)== noErr) {
	SendEventToEventTarget (theEvent, theTarget);
	ReleaseEvent(theEvent);
    }
*/
