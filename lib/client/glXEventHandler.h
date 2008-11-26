/* Copyright (c) 2006-2008, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQ_GLXEVENTHANDLER_H
#define EQ_GLXEVENTHANDLER_H

#include <eq/client/eventHandler.h> // base class
#include <eq/client/types.h>        // basic typedefs
#include <eq/client/windowSystem.h> // XEvent type

#include <eq/net/connectionSet.h>

namespace eq
{
    class Pipe;
    class Window;
    class GLXWindowEvent;

    /**
     * The event handler for glX.
     */
    class GLXEventHandler : public EventHandler
    {
    public:
        class EventSet : public net::ConnectionSet, public base::Referenced
        {
        public:
            void notifyPerThreadDelete() { unref(); }

        protected:
            virtual ~EventSet(){}
        };

        /** Dispatch at least one event for the current thread, blocking. */
        static void dispatchOne();

        /** Dispatch all pending events on the current thread, non-blocking. */
        static void dispatchAll();

        /** Get the event set of the current thread. */
        static base::RefPtr< EventSet > getEventSet();

        /** Clear the event set of the current thread. */
        static void clearEventSet();


        /** Constructs a new glx event handler. */
        GLXEventHandler( Pipe* pipe );

        /** @sa EventHandler::deregisterPipe. */
        virtual void deregisterPipe( Pipe* pipe );

    private:
        /** Destructs the glX event handler. */
        virtual ~GLXEventHandler();
        
        static void _handleEvents( X11ConnectionPtr connection );

        void _processEvent( GLXWindowEvent& event, Pipe* pipe );
        uint32_t  _getButtonState( XEvent& event );
        uint32_t  _getButtonAction( XEvent& event );
        uint32_t  _getKey( XEvent& event );
    };

    typedef base::RefPtr< GLXEventHandler::EventSet > GLXEventSetPtr; 
}

#endif // EQ_GLXEVENTHANDLER_H

