
/* Copyright (c) 2005-2009, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQ_CHANNEL_H
#define EQ_CHANNEL_H

#include <eq/client/colorMask.h>     // enum used
#include <eq/client/event.h>         // member
#include <eq/client/eye.h>           // enum used
#include <eq/client/frame.h>         // used in inline method
#include <eq/client/pixelViewport.h> // member
#include <eq/client/window.h>        // used in inline method
#include <vmmlib/vmmlib.h>           // Frustum definition 

namespace eq
{
    class Pixel;
    class Node;
    class Range;
    class SceneObject;
    class FrameBufferObject;
    struct RenderContext;

    /**
     * A channel represents a two-dimensional viewport within a Window.
     *
     * The channel is the basic rendering entity. It executes all
     * rendering-relevant tasks, such as clear, draw, assemble and readback. It
     * is a child of a Window.
     */
    class EQ_EXPORT Channel : public net::Object
    {
    public:
    
        /** 
         * The drawable format defining the components used as an alternate
         * drawable for this cannel.
         */
        enum Drawable
        {
            FBO_NONE      = EQ_BIT_NONE,
            FBO_COLOR     = EQ_BIT1,  //!< Use FBO color attachment
            FBO_DEPTH     = EQ_BIT2,  //!< Use FBO depth attachment
            FBO_STENCIL   = EQ_BIT3   //!< Use FBO stencil attachment
        };
        
        /** Constructs a new channel. */
        Channel( Window* parent );

        /** Destructs the channel. */
        virtual ~Channel();

        /**
         * @name Data Access
         */
        //*{
        Window* getWindow() const { return _window; }
        Pipe*   getPipe() const { return ( _window ? _window->getPipe() : 0 );}
        Node* getNode() const { return ( _window ? _window->getNode() : 0 );}
        Config* getConfig() const
            { return ( _window ? _window->getConfig() : 0 );}
        ServerPtr getServer() const
            { return ( _window ? _window->getServer() : 0 );}

        /** 
         * Get the GLEW context for this channel.
         * 
         * The glew context is initialized during window initialization, and
         * provides access to OpenGL extensions. This function does not follow
         * the Equalizer naming conventions, since GLEW uses a function of this
         * name to automatically resolve OpenGL function entry
         * points. Therefore, any supported GL function can be called directly
         * from an initialized Channel.
         * 
         * @warning Not finalized, might change in the future.
         * @return the extended OpenGL function table for the channel's OpenGL
         *         context.
         */
        GLEWContext* glewGetContext() { return _window->glewGetContext(); }
        const GLEWContext* glewGetContext() const { return _window->glewGetContext(); }
        
        /** @return the name of the window. */
        const std::string& getName() const { return _name; }

        /** 
         * Return the set of tasks this channel might execute in the worst case.
         * 
         * It is not guaranteed that all the tasks will be actually executed
         * during rendering.
         * 
         * @return the tasks.
         */
        uint32_t getTasks() const { return _tasks; }

        /** 
         * Traverse this channel and all children using a channel visitor.
         * 
         * @param visitor the visitor.
         * @return the result of the visitor traversal.
         */
        ChannelVisitor::Result accept( ChannelVisitor* visitor )
            { return visitor->visit( this ); }

        /** 
         * Set the near and far planes for this channel.
         * 
         * The near and far planes are set during initialisation and are
         * inherited by source channels contributing to the rendering of this
         * channel. Dynamic near and far planes can be applied using
         * applyNearFar.
         *
         * @param nearPlane the near plane.
         * @param farPlane the far plane.
         */
        void setNearFar( const float nearPlane, const float farPlane );

        /** Return a stable, unique color for this channel. */
        const vmml::Vector3ub& getUniqueColor() const { return _color; }

        /** 
         * Get the channel's view.
         * 
         * A channel has a View if a Wall or Projection description is
         * configured for it. This is typically the case for destination
         * channels, source channels do not have a view.
         * 
         * @return the channel's view, or 0 if it does not have a view.
         */
        const View* getView() const { return _view; }

        /** Add a new statistics event for the current frame. */
        void addStatistic( Event& event );
        //*}

        /**
         * @name Context-specific data access.
         * 
         * The data returned by these methods depends on the context (callback)
         * they are called from, typically the data for the current rendering
         * task.
         */
        //*{
        /** @return the channel's current draw buffer. */
        uint32_t getDrawBuffer() const;

        /** @return the channel's current read buffer. */
        uint32_t getReadBuffer() const;

        /** @return the channel's current color mask for drawing. */
        const ColorMask& getDrawBufferMask() const;

        /** @return the channel's current pixel viewport. */
        const PixelViewport& getPixelViewport() const;

        /**
         * Get the channel's current position wrt the destination channel.
         *
         * Note that computing this value from the current viewport and pixel
         * viewport inaccurate because it neglects rounding of the pixel
         * viewport done by the server.
         *
         * @return the channel's current position wrt the destination channel.
         */
        const vmml::Vector2i& getPixelOffset() const;

        /** @return the perspectivefrustum for the current rendering task. */
        const vmml::Frustumf& getFrustum() const;

        /** @return the orthographics frustum for the current rendering task. */
        const vmml::Frustumf& getOrtho() const;

        /** @return the fractional viewport wrt the destination. */
        const Viewport& getViewport() const;

        /** @return the database range for the current rendering task. */
        const Range& getRange() const;

        /** @return the pixel decomposition for the current rendering task. */
        const Pixel& getPixel() const;

        /** @return the currently rendered eye pass. */
        Eye getEye() const;

        /**
         * @return the view transformation to position and orient the view
         *         frustum.
         */
        const vmml::Matrix4f& getHeadTransform() const;

        /** @return the list of input frames, used from frameAssemble(). */
        const FrameVector& getInputFrames() { return _inputFrames; }

        /** @return the list of output frames, used from frameReadback(). */
        const FrameVector& getOutputFrames() { return _outputFrames; }

        /** @return the position of this channel wrt the 2D virtual screen. */
        const vmml::Vector2i& getScreenOrigin() const;

        /** @return the size of the 2D virtual screen. */
        vmml::Vector2i getScreenSize() const;

        /** 
         * @return the 2D orthographic frustum for the 2D virtual screen.
         * @sa getScreenOrigin, getPixelViewport, getPixel
         */
        vmml::Frustumf getScreenFrustum() const;
        
        /** 
         * get the channel's native (drawable) pixel viewport.
         */
        const eq::PixelViewport& getNativePixelViewPort() const { return _pvp; }

        /** 
         * get the FBO used as an alternate frame buffer.
         */
        FrameBufferObject* getFrameBufferObject();
        //*}

        /**
         * @name Operations
         *
         * Operations are only meaningfull from within certain callbacks. They
         * are just convenience wrappers applying context-specific data to the
         * OpenGL state.
         */
        //*{
        /** 
         * Apply the current rendering buffer, including the color mask.
         */
        virtual void applyBuffer();

        /** 
         * Apply the current color mask.
         */
        virtual void applyColorMask() const;

        /** 
         * Apply the OpenGL viewport for the current rendering task.
         */
        virtual void applyViewport() const;

        /**
         * Apply the perspective frustum matrix for the current rendering task.
         */
        virtual void applyFrustum() const;

        /**
         * Apply the orthographic frustum matrix for the current rendering task.
         */
        virtual void applyOrtho() const;

        /** 
         * Apply the modelling transformation to position and orient the view
         * frustum.
         */
        virtual void applyHeadTransform() const;

        /** 
         * Apply the current alternate frame buffer.
         */
        virtual void applyFrameBufferObject();
        
        /** 
         * Process a received event.
         *
         * The task of this method is to update the channel as necessary, and 
         * transform the event into an config event to be send to the 
         * application using Config::sendEvent().
         * 
         * @param event the received event.
         * @return true when the event was handled, false if not.
         */
        virtual bool processEvent( const Event& event );

        /** Draw a statistics overlay. */
        virtual void drawStatistics();

        /** Outlines the current pixel viewport. */
        virtual void outlineViewport();

        //*{
        /** 
         * Apply a orthographic frustum for the 2D virtual screen. 
         *
         * One unit matches one 2D virtual screen pixel. The frustum is
         * positioned this channel's virtual screen position. The z-range is
         * [-1,1].
         */
        void applyScreenFrustum() const;
        //*}

        /**
         * @name Attributes
         */
        //*{
        // Note: also update string array initialization in channel.cpp
        enum IAttribute
        {
            IATTR_HINT_STATISTICS,
            IATTR_ALL
        };
        
        int32_t  getIAttribute( const IAttribute attr ) const
            { return _iAttributes[attr]; }
        static const std::string&  getIAttributeString( const IAttribute attr )
            { return _iAttributeStrings[attr]; }
        //*}
#if 0
        /** @name Scene Object Access. */
        //*{
        SceneObject* getNextSceneObject();
        SceneObject* checkNextSceneObject();
        //void putSceneObject( SceneObject* object );
        void passSceneObject( SceneObject* object );
        void flushSceneObjects();
        //*}
#endif

    protected:
        friend class Window;

        void attachToSession( const uint32_t id, 
                              const uint32_t instanceID, 
                              net::Session* session );

        /** @name Actions */
        //*{
        /** 
         * Start a frame by unlocking all child resources.
         * 
         * @param frameNumber the frame to start.
         */
        void startFrame( const uint32_t frameNumber ) { /* currently nop */ }

        /** 
         * Signal the completion of a frame to the parent.
         * 
         * @param frameNumber the frame to end.
         */
        void releaseFrame( const uint32_t frameNumber ) { /* currently nop */ }

        /** 
         * Release the local synchronization of the parent for a frame.
         * 
         * @param frameNumber the frame to release.
         */
        void releaseFrameLocal( const uint32_t frameNumber ) { /* nop */ }
        //*}

        /**
         * @name Callbacks
         *
         * The callbacks are called by Equalizer during rendering to execute
         * various actions.
         */
        //*{
        /** 
         * Initialize this channel.
         * 
         * @param initID the init identifier.
         */
        virtual bool configInit( const uint32_t initID );

        /** 
         * Exit this channel.
         */
        virtual bool configExit();

        /** 
         * Rebind the current alternate rendering buffer.
         */
        void makeCurrent();
        
        /**
         * Start rendering a frame.
         *
         * Called once at the beginning of each frame, to do per-frame updates
         * of channel-specific data. This method has to call startFrame().
         *
         * @param frameID the per-frame identifier.
         * @param frameNumber the frame to start.
         * @sa Config::beginFrame()
         */
        virtual void frameStart( const uint32_t frameID, 
                                 const uint32_t frameNumber ) 
            { startFrame( frameNumber ); }

        /**
         * Finish rendering a frame.
         *
         * Called once at the end of each frame, to do per-frame updates of
         * channel-specific data.  This method has to call releaseFrame().
         *
         * @param frameID the per-frame identifier.
         * @param frameNumber the frame to finish.
         */
        virtual void frameFinish( const uint32_t frameID, 
                                  const uint32_t frameNumber ) 
            { releaseFrame( frameNumber ); }

        /** 
         * Finish drawing.
         * 
         * Called once per frame after the last draw operation. Typically
         * releases the local node thread synchronization for this frame.
         *
         * @param frameID the per-frame identifier.
         * @param frameNumber the frame to finished with draw.
         */
        virtual void frameDrawFinish( const uint32_t frameID, 
                                      const uint32_t frameNumber )
            { releaseFrameLocal( frameNumber ); }

        /** 
         * Clear the frame buffer.
         * 
         * @param frameID the per-frame identifier.
         */
        virtual void frameClear( const uint32_t frameID );

        /** 
         * Draw the scene.
         * 
         * @param frameID the per-frame identifier.
         */
        virtual void frameDraw( const uint32_t frameID );

        /** 
         * Assemble input frames.
         * 
         * @param frameID the per-frame identifier.
         * @sa getInputFrames
         */
        virtual void frameAssemble( const uint32_t frameID );

        /** 
         * Read back the rendered scene.
         * 
         * @param frameID the per-frame identifier.
         * @sa getOutputFrames
         */
        virtual void frameReadback( const uint32_t frameID );

        /**
         * Setup the OpenGL state for a readback or assemble operation.
         *
         * The default implementation is very conservative and saves any state
         * which is potentially changed by the assembly routines.
         */
        virtual void setupAssemblyState();

        /**
         * Reset the OpenGL state after an assembly operation.
         */
        virtual void resetAssemblyState();
        //*}

        /** @name Error information. */
        //*{
        /** 
         * Set a message why the last operation failed.
         * 
         * The message will be transmitted to the originator of the request, for
         * example to Config::init when set from within the configInit method.
         *
         * @param message the error message.
         */
        void setErrorMessage( const std::string& message ) { _error = message; }
        //*}
    private:
        //-------------------- Members --------------------
        /** The parent window. */
        Window* const _window;

        /** The name. */
        std::string    _name;
        
        /** A unique color assigned by the server during config init. */
        vmml::Vector3ub _color;

        /** The reason for the last error. */
        std::string     _error;

        /** Integer attributes. */
        int32_t _iAttributes[IATTR_ALL];
        /** String representation of integer attributes. */
        static std::string _iAttributeStrings[IATTR_ALL];

        /** Worst-case set of tasks. */
        uint32_t _tasks;

        /** server-supplied rendering data. */
        RenderContext* _context;

        /** server-supplied vector of output frames for current task. */
        FrameVector _outputFrames;

        /** server-supplied vector of input frames for current task. */
        FrameVector _inputFrames;

        /** The native pixel viewport wrt the window. */
        eq::PixelViewport _pvp;

        /** The native viewport. */
        Viewport       _vp;

        /** true if the pvp is immutable, false if the vp is immutable */
        bool _fixedPVP;

        /** The native perspective ('identity') frustum. */
        vmml::Frustumf  _frustum;

        /** The native orthographic ('identity') frustum. */
        vmml::Frustumf  _ortho;

        /** Used as an alternate drawable. */       
        FrameBufferObject* _fbo; 
        
        /** Alternate drawable definition. */
        uint32_t _drawable;
        
        /** The statistics events gathered during the current frame. */
        std::vector< Statistic > _statistics;
#ifdef EQ_ASYNC_TRANSMIT
        base::SpinLock _statisticsLock;
#endif

        /** The channel's view, if it has one. */
        View* _view;

        /** Initialize the FBO */
        bool _configInitFBO();
        
        //-------------------- Methods --------------------
        /** 
         * Set the channel's fractional viewport wrt its parent pipe.
         *
         * Updates the pixel viewport accordingly.
         * 
         * @param vp the fractional viewport.
         */
        void _setViewport( const Viewport& vp );

        /** 
         * Set the channel's pixel viewport wrt its parent pipe.
         *
         * Updates the fractional viewport accordingly.
         * 
         * @param pvp the viewport in pixels.
         */
        void _setPixelViewport( const PixelViewport& pvp );

        /** Notification of window pvp change. */
        void _notifyViewportChanged();

        /** Setup the current rendering context. */
        void _setRenderContext( RenderContext& context );

        virtual void getInstanceData( net::DataOStream& os ) { EQDONTCALL }
        virtual void applyInstanceData( net::DataIStream& is ) { EQDONTCALL }

        /* The command handler functions. */
        net::CommandResult _cmdConfigInit( net::Command& command );
        net::CommandResult _cmdConfigExit( net::Command& command );
        net::CommandResult _cmdFrameStart( net::Command& command );
        net::CommandResult _cmdFrameFinish( net::Command& command );
        net::CommandResult _cmdFrameClear( net::Command& command );
        net::CommandResult _cmdFrameDraw( net::Command& command );
        net::CommandResult _cmdFrameDrawFinish( net::Command& command );
        net::CommandResult _cmdFrameAssemble( net::Command& command );
        net::CommandResult _cmdFrameReadback( net::Command& command );
        net::CommandResult _cmdFrameTransmit( net::Command& command );
    };
}

#endif // EQ_CHANNEL_H

