
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@googlemail.com>
 *               2012, Stefan Eilemann <eile@eyescale.ch>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CO_OBJECTREGISTRY_H
#define CO_OBJECTREGISTRY_H

#include <co/serializable.h>      // base class
#include <co/objectFactory.h>

namespace co
{
namespace detail { class ObjectMap; }

    /** Central distributed object registry. */
    class ObjectMap : public Serializable
    {
    public:
        /**
         * Construct a new ObjectMap.
         *
         * @param localNode used for object registering and mapping
         * @param factory to create & destroy slave objects
         * @version 0.5.1
         */
        ObjectMap( LocalNodePtr localNode, ObjectFactory& factory );

        /**
         * Destroy an ObjectMap.
         *
         * All registered and mapped objects will be deregistered and unmapped.
         * All mapped objects will be destroyed using the object factory.
         * @version 0.5.1
         */
        virtual ~ObjectMap();

        /**
         * Add and register a new object as master instance to this objectMap.
         *
         * Upon registering in the localNode, this object will be remembered for
         * serialization on the next call to commit.
         *
         * @param object the new object to add and register
         * @param type unique object type to create object via slave factory
         * @return false on failed registerObject in localNode, true otherwise
         * @version 0.5.1
         */
        bool register_( Object* object, const uint32_t type );

        /**
         * Map and return an object.
         *
         * The object is either created via its type specified upon registering
         * or an already created instance is used if passed to this function.
         * The object will be mapped to the version that was current on
         * registration time.
         *
         * @param identifier unique object identifier used for map operation
         * @param instance already created instance to skip factory creation
         * @return 0 if not registered, the valid instance otherwise
         * @version 0.5.1
         */
        Object* get( const uint128_t& identifier, Object* instance=0 );

        /** Commits all registered objects. @version 0.5.1 */
        virtual uint128_t commit( const uint32_t incarnation = CO_COMMIT_NEXT );

    protected:
        virtual bool isDirty() const;

        virtual void serialize( DataOStream& os, const uint64_t dirtyBits );
        virtual void deserialize( DataIStream& is, const uint64_t dirtyBits );

        virtual ChangeType getChangeType() const { return UNBUFFERED; }

        /** The changed parts of the object since the last serialize(). */
        enum DirtyBits
        {
            DIRTY_ADDED       = Serializable::DIRTY_CUSTOM << 0, // 1
            DIRTY_CHANGED     = Serializable::DIRTY_CUSTOM << 1, // 2
            DIRTY_CUSTOM      = Serializable::DIRTY_CUSTOM << 2  // 4
        };        

    private:
        detail::ObjectMap* const _impl;

        /** Commit and note new master versions. */
        void _commitMasters( const uint32_t incarnation );
    };
}
#endif // CO_OBJECTREGISTRY_H