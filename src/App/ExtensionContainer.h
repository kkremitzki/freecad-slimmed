/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef APP_EXTENSIONCONTAINER_H
#define APP_EXTENSIONCONTAINER_H

#include "Extension.h"
#include "PropertyContainer.h"
#include "PropertyPythonObject.h"
#include "DynamicProperty.h"
#include <CXX/Objects.hxx>

#include <boost/preprocessor/seq/for_each.hpp>

namespace App {
    
/**
 * @brief Container which can hold extensions
 * 
 * In FreeCAD normally inheritance is a chain, it is not possible to use multiple inheritance. 
 * The reason for this is that all objects need to be exposed to python, and it is basically 
 * impossible to handle multiple inheritance in the C-API for python extensions. Also using multiple
 * parent classes in python is currently not possible with the default object aproach.
 * 
 * The concept of extensions allow to circumvent those problems. Extensions are FreeCAD objects 
 * which work like normal objects in the sense that they use properties and class methods to define 
 * their functionality. However, they are not exposed as individual usable entities but are used to 
 * extend other objects. A extended object gets all the properties and methods of the extension. 
 * Therefore it is like c++ multiple inheritance, which is indeed used to achieve this on c++ side, 
 * but provides a few important additional functionalities:
 * - Property persistance is handled, save and restore work out of the box
 * - The objects python API gets extended too with the extension python API
 * - Extensions can be added from c++ and python, even from both together
 *
 * The interoperability with python is highly important, as in FreeCAD all functionality should be 
 * as easily accessible from python as from c++. To ensure this, and as already noted, extensions can 
 * be added to a object from python. However, this means that it is not clear from the c++ object type
 * if an extension was added or not. If added from c++ it becomes clear in the type due to the use of
 * multiple inheritance. If added from python it is a runtime extension and not visible from type. 
 * Hence querying existing extensions of an object and accessing its methods works not by type 
 * casting but by the interface provided in ExtensionContainer. The default workflow is to query if 
 * an extension exists and then get the extension obejct. No matter if added from python or c++ this 
 * interface works always the same. 
 * @code
 * if (object->hasExtension(GroupExtension::getClassTypeId())) {
 *     App::GroupExtension* group = object->getExtensionByType<GroupExtension>();
 *     group->hasObject(...); 
 * }
 * @endcode
 * 
 * To add a extension to an object, it must comply to a single restriction: it must be derived 
 * from ExtensionContainer. This is important to allow adding extensions from python and also to 
 * access the universal extension API. As DocumentObject itself derives from ExtensionContainer this 
 * should be the case automatically in most circumstances. 
 * 
 * Note that a small boilerplate change is needed next to the multiple inheritance when adding 
 * extensions from c++. It must be ensured that the type registration is aware of the extensions.
 * Here a working example:
 * @code 
 * class AppExport Part : public App::DocumentObject, public App::FirstExtension, public App::SecondExtension {
 *   PROPERTY_HEADER_WITH_EXTENSIONS(App::Part);
 * };
 * PROPERTY_SOURCE_WITH_EXTENSIONS(App::Part, App::DocumentObject, (App::FirstExtension)(App::SecondExtension))
 * Part::Part(void) {
 *   FirstExtension::initExtension(this);
 *   SecondExtension::initExtension(this);
 * }
 * @endcode
 * 
 * From python adding an extension is easier, it must be simply registered to a document object 
 * at object initialisation like done with proeprties. Note that the special python extension objects
 * need to be added, not the c++ objects. Normally the only difference in name is the additional 
 * "Python" at the end of the extension name.
 * @code{.py}
 * class Test():
 *   __init(self)__:
 *     registerExtension("App::FirstExtensionPython", self)
 *     registerExtension("App::SecondExtensionPython", self)
 * @endcode
 * 
 * Extensions can provide methods that should be overriden by the extended object for customisation
 * of the extension behaviour. In c++ this is as simple as overriding the provided virtual functions.
 * In python a class method must be provided which has the same name as the method to override. This 
 * method must not neccessarily be in the object that is extended, it must be in the object which is 
 * provided to the "registerExtension" call as second argument. This second argument is used as a 
 * proxy and enqueired if the method to override exists in this proxy before calling it. 
 * 
 * For information on howto create extension see the documentation of Extension
 */
class AppExport ExtensionContainer : public virtual App::PropertyContainer
{

    TYPESYSTEM_HEADER();

public:
    
    typedef std::map<Base::Type, App::Extension*>::iterator ExtensionIterator;

    ExtensionContainer();
    virtual ~ExtensionContainer();

    void registerExtension(Base::Type extension, App::Extension* ext);
    bool hasExtension(Base::Type) const;
    bool hasExtension(const char* name) const; //this version does not check derived classes
    App::Extension* getExtension(Base::Type);
    App::Extension* getExtension(const char* name); //this version does not check derived classes
    template<typename ExtensionT>
    ExtensionT* getExtensionByType() {
        return dynamic_cast<ExtensionT*>(getExtension(ExtensionT::getClassTypeId()));
    };
    
    //get all extensions which have the given base class
    std::vector<Extension*> getExtensionsDerivedFrom(Base::Type type) const;
    template<typename ExtensionT>
    std::vector<ExtensionT*> getExtensionsDerivedFromType() const {
        auto vec = getExtensionsDerivedFrom(ExtensionT::getClassTypeId());
        std::vector<ExtensionT*> typevec;
        for(auto ext : vec)
            typevec.push_back(dynamic_cast<ExtensionT*>(ext));
        
        return typevec;
    };
    
    ExtensionIterator extensionBegin() {return _extensions.begin();};
    ExtensionIterator extensionEnd() {return _extensions.end();};
       
    
    /** @name Access properties */
    //@{
    /// find a property by its name
    virtual Property *getPropertyByName(const char* name) const override;
    /// get the name of a property
    virtual const char* getPropertyName(const Property* prop) const override;
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyMap(std::map<std::string,Property*> &Map) const override;
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyList(std::vector<Property*> &List) const override;

    /// get the Type of a Property
    virtual short getPropertyType(const Property* prop) const override;
    /// get the Type of a named Property
    virtual short getPropertyType(const char *name) const override;
    /// get the Group of a Property
    virtual const char* getPropertyGroup(const Property* prop) const override;
    /// get the Group of a named Property
    virtual const char* getPropertyGroup(const char *name) const override;
    /// get the Group of a Property
    virtual const char* getPropertyDocumentation(const Property* prop) const override;
    /// get the Group of a named Property
    virtual const char* getPropertyDocumentation(const char *name) const override;
    //@}
    
    virtual void onChanged(const Property*);
    
private:
    //stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};


#define PROPERTY_HEADER_WITH_EXTENSIONS(_class_) \
  PROPERTY_HEADER(_class)

//helper macro to add parent to property data
#define ADD_PARENT(r, data, elem)\
    data::propertyData.parentPropertyData.push_back(elem::getPropertyDataPtr());

/// 
#define PROPERTY_SOURCE_WITH_EXTENSIONS(_class_, _parentclass_, _extensions_) \
TYPESYSTEM_SOURCE_P(_class_);\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  ADD_PARENT(0, _class_, _parentclass_)\
  BOOST_PP_SEQ_FOR_EACH(ADD_PARENT, _class_, _extensions_)\
}

} //App

#endif // APP_EXTENSIONCONTAINER_H
