/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_MODULE3D_STATE_HPP
#define DCM_MODULE3D_STATE_HPP

#include "module.hpp"
#include <opendcm/moduleState/traits.hpp>
#include <opendcm/core/clustergraph.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function/adapt_function.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/greater.hpp>

#include <ios>

namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace karma_ascii = boost::spirit::karma::ascii;
namespace qi_ascii = boost::spirit::qi::ascii;
namespace phx = boost::phoenix;

namespace dcm {
  
namespace details {
   
template<typename Sys>
struct getModule3D {
    typedef typename system_traits<Sys>::template getModule<m3d>::type type;
};

    
struct geom_visitor : public boost::static_visitor<std::string> {
   
    template<typename T>
    std::string operator()(T& i) const {
      
	//we use stings in case new geometry gets added and the weights shift, meaning: backwards 
	//compatible
        std::string type;
	switch( geometry_traits<T>::tag::weight::value ) {
	  case tag::weight::direction::value :
	    return "direction";
	  case tag::weight::point::value :
	    return "point";
	  case tag::weight::line::value :
	    return "line";
	  case tag::weight::plane::value :
	    return "plane";
	  case tag::weight::cylinder::value :
	    return "cylinder";
	  default:
	    return "unknown";
	};
    };
}; 

template<typename T>
std::string getWeight(boost::shared_ptr<T> ptr) {
    geom_visitor v;
    return ptr->apply(v);
};

template<typename T>
struct get_weight {
  typedef typename geometry_traits<T>::tag::weight type;
};

//search the first type in the typevector with the given weight
template<typename Vector, typename Weight>
struct getWeightType {
  typedef typename mpl::find_if<Vector, boost::is_same<get_weight<mpl::_1>, Weight > >::type iter;
  typedef typename mpl::deref<iter>::type type;
};

typedef std::vector< fusion::vector2<std::string, std::string> > string_vec;
typedef std::vector< fusion::vector2<std::vector<char>, std::vector<char> > > char_vec;

template<typename C>
string_vec getConstraints(boost::shared_ptr<C> con) {
    
    string_vec vec;
    std::vector<boost::any> cvec = con->getGenericConstraints();
    
    typename std::vector<boost::any>::iterator it;
    for(it = cvec.begin(); it != cvec.end(); it++) {
      
      if((*it).type() == typeid(dcm::Distance)) {
	std::string value = boost::lexical_cast<std::string>(boost::any_cast<dcm::Distance>(*it).value);
	vec.push_back(fusion::make_vector(std::string("Distance"), value));
      }
      else if((*it).type() == typeid(dcm::Angle)) {
	std::string value = boost::lexical_cast<std::string>(boost::any_cast<dcm::Angle>(*it).value);
	vec.push_back(fusion::make_vector(std::string("Angle"), value));
      }
      else if((*it).type() == typeid(dcm::Orientation)) {
	std::string value = boost::lexical_cast<std::string>(boost::any_cast<dcm::Orientation>(*it).value);
	vec.push_back(fusion::make_vector(std::string("Orientation"), value));
      };
    };
    return vec;   
};

template<typename Seq, typename T>
struct push_seq {
    typedef typename fusion::result_of::as_vector<typename mpl::push_back< Seq, T >::type>::type type;
};

template<typename State, typename Add>
typename push_seq<State, Add>::type append(State& s, const typename Add::option_type& val) {

        typedef typename push_seq<State, Add>::type Sequence;
        typedef typename fusion::result_of::begin<Sequence>::type Begin;
        typedef typename fusion::result_of::end<Sequence>::type End;
        typedef typename fusion::result_of::prior<End>::type EndOld;

        //create the new sequence
        Sequence vec;

        //copy the old values into the new sequence
        Begin b(vec);
        EndOld eo(vec);

        fusion::iterator_range<Begin, EndOld> range(b, eo);
        fusion::copy(s, range);

        //insert this object at the end of the sequence
        fusion::back(vec) = val;

        //and return our new extendet sequence
        return vec;
};

template<typename State, typename C, typename Count>
typename boost::enable_if<typename mpl::greater<Count, mpl::int_<3> >::type, void>::type
recursiveCreation(typename char_vec::iterator it,
		       typename char_vec::iterator end,
		       boost::shared_ptr<C> con,
		       State s		      ) {};

template<typename State, typename C, typename Count>
typename boost::enable_if<typename mpl::less_equal<Count, mpl::int_<3> >::type, void>::type 
recursiveCreation(typename char_vec::iterator it,
		       typename char_vec::iterator end,
		       boost::shared_ptr<C> con,
		       State s		      ) {
  
  if(it == end) {
    con->template initialize<State>(s);
    return;
  };
  
  std::string first( fusion::at_c<0>(*it).begin(), fusion::at_c<0>(*it).end() );
  std::string second( fusion::at_c<1>(*it).begin(), fusion::at_c<1>(*it).end() );
  
  if( first.compare("Distance") == 0 ) {
    typedef typename push_seq<State, dcm::Distance>::type Vec;
    Vec vec = append<State, dcm::Distance>(s, boost::lexical_cast<typename dcm::Distance::option_type>(second));
    recursiveCreation<Vec, C, typename mpl::next<Count>::type >(++it, end, con, vec);
    return;
  };
};

template<typename C>
void setConstraints(char_vec& vec, boost::shared_ptr<C> con ) {
    recursiveCreation<fusion::vector<>, C, mpl::int_<0> >(vec.begin(), vec.end(), con, fusion::vector<>());
};

template <typename Geom, typename Row, typename Value>
bool VectorOutput(Geom &v, Row& r, Value& val) {
	  
            if (r < v->m_global.rows()) {
	      
                val = v->m_global(r++);
                return true; // output continues
            }
            return false;    // fail the output
};

template <typename Geom, typename Row, typename Value>
bool VectorInput(Geom &v, Row& r, Value& val) {
	    
	    v.conservativeResize(r+1);
            v(r++) = val;
            return true; // output continues
};

template<typename Geom>
struct inject_set {

    template<typename Vec, typename Obj>
    static void apply(Vec& v, Obj g) {
	Geom gt;
	(typename geometry_traits<Geom>::modell()).template inject<double,
		  typename geometry_traits<Geom>::accessor >(gt, v);
	g->set(gt);
    };
};
//spezialisation if no type in the typelist has the right weight
template<>
struct inject_set<mpl_::void_> {

    template<typename Obj, typename Vec>
    static void apply(Vec& v, Obj g) {
      //TODO:throw   
    };
};

template<typename System>
bool Create(System* sys, std::string& type, 
	      boost::shared_ptr<typename details::getModule3D<System>::type::Geometry3D> geom, 
	      typename System::Kernel::Vector& v) {
  
  typedef typename details::getModule3D<System>::type::geometry_types Typelist;
  
  if(type.compare("direction") == 0 ) {
    inject_set<typename getWeightType<Typelist, tag::weight::direction>::type>::apply(v, geom);
  }  
  else if(type.compare("point") == 0) {
    inject_set<typename getWeightType<Typelist, tag::weight::point>::type>::apply(v, geom);
  }
  else if(type.compare("line") == 0) {
    inject_set<typename getWeightType<Typelist, tag::weight::line>::type>::apply(v, geom);
  }
  else if(type.compare("plane") == 0 ) {
    inject_set<typename getWeightType<Typelist, tag::weight::plane>::type>::apply(v, geom);
  }
  else if(type.compare("cylinder") == 0 ) {
    inject_set<typename getWeightType<Typelist, tag::weight::cylinder>::type>::apply(v, geom);
  };
  return true;
};

// define a new real number formatting policy
template <typename Num>
struct scientific_policy : karma::real_policies<Num>
{
    // we want the numbers always to be in scientific format
    static int floatfield(Num n) { return std::ios::scientific; }
    static unsigned precision(Num n) {return 16;};
};

// define a new generator type based on the new policy
typedef karma::real_generator<double, scientific_policy<double> > science_type;
static science_type const scientific = science_type();
} //details
} //dcm
 
BOOST_PHOENIX_ADAPT_FUNCTION( bool, vector_out, dcm::details::VectorOutput, 3)
BOOST_PHOENIX_ADAPT_FUNCTION( bool, vector_in,  dcm::details::VectorInput, 3)
BOOST_PHOENIX_ADAPT_FUNCTION( bool, create,  dcm::details::Create, 4)
 
BOOST_FUSION_ADAPT_STRUCT(
    dcm::GlobalEdge,
    (int, ID)
    (int, source)
    (int, target)
)

namespace dcm {  
  
template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::Geometry3D , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::Geometry3D , System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry;
    typedef karma::rule<iterator, boost::shared_ptr<Geometry>(), karma::locals<int> > generator;
    static void init(generator& r) {
       r = karma::lit("<type>Geometry3D</type>\n<class>")
	    << karma_ascii::string[karma::_1 = phx::bind(&details::getWeight<Geometry>, karma::_val)]
	    << "</class>" << karma::eol << "<value>" 
	    << (details::scientific[ boost::spirit::_pass = vector_out(karma::_val, karma::_a, karma::_1) ] % ' ')
	    << "</value>";
    };
};


template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::vertex_prop , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::vertex_prop , System, iterator > {

    typedef karma::rule<iterator, GlobalVertex()> generator;
    static void init(generator& r) {
        r = karma::lit("<type>Vertex</type>")
	    << karma::eol << "<value>" << karma::int_ << "</value>";
    };
};

template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::Constraint3D , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::Constraint3D , System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry3D;
    typedef typename details::getModule3D<System>::type::Constraint3D  Constraint3D;
    typedef typename details::getModule3D<System>::type::vertex_prop vertex_prop;
    typedef karma::rule<iterator, boost::shared_ptr<Constraint3D>()> generator;
    static void init(generator& r) {
       r = karma::lit("<type>Constraint3D</type>") << karma::eol
	   << "<connect first=" << karma::int_[karma::_1 = phx::bind(&Geometry3D::template getProperty<vertex_prop>, phx::bind(&Constraint3D::first, karma::_val))]
	   << " second=" << karma::int_[karma::_1 = phx::bind(&Geometry3D::template getProperty<vertex_prop>, phx::bind(&Constraint3D::second, karma::_val))] << "></connect>"
	   << (*(karma::eol<<"<constraint type="<<karma_ascii::string<<">"<<karma_ascii::string<<"</constraint>"))[karma::_1 = phx::bind(&details::getConstraints<Constraint3D>, karma::_val)];
    };
};

template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::edge_prop , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::edge_prop , System, iterator > {

    typedef karma::rule<iterator, GlobalEdge&()> generator;
    static void init(generator& r) {
        r %= karma::lit("<type>Edge</type>")
	    << karma::eol << "<value>" << karma::int_ << " " 
	    << karma::int_ << " " << karma::int_ << "</value>";
    };
};

template<typename System>
struct parser_generate<typename details::getModule3D<System>::type::fix_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<typename details::getModule3D<System>::type::fix_prop, System, iterator> {
    typedef karma::rule<iterator, bool&()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>Fix</type>\n<value>") << karma::bool_ <<"</value>";
    };
};

/****************************************************************************************************/
/****************************************************************************************************/

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::Geometry3D , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::Geometry3D, System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  object_type;
    typedef typename System::Kernel Kernel;
    
    typedef qi::rule<iterator, boost::shared_ptr<object_type>(System*), qi::space_type, qi::locals<std::string, typename Kernel::Vector, int> > parser;
    static void init(parser& r) {
        r = qi::lit("<type>Geometry3D</type>")[ qi::_val =  phx::construct<boost::shared_ptr<object_type> >( phx::new_<object_type>(*qi::_r1))]
		  >> "<class>" >> (+qi::char_("a-zA-Z"))[qi::_a = phx::construct<std::string>(phx::begin(qi::_1), phx::end(qi::_1))] >> "</class>"
		  >> "<value>" >> *qi::double_[ vector_in(qi::_b, qi::_c, qi::_1) ] >> "</value>"
		  >> qi::eps[ create(qi::_r1, qi::_a, qi::_val, qi::_b) ];
    };
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::vertex_prop, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::vertex_prop, System, iterator > {
  
    typedef qi::rule<iterator, GlobalVertex(), qi::space_type> parser;
    static void init(parser& r) {
        r %= qi::lit("<type>Vertex</type>") >> "<value>" >> qi::int_ >> "</value>";
    };
};


template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::Constraint3D , System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::Constraint3D, System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry3D;
    typedef typename details::getModule3D<System>::type::Constraint3D  Constraint3D;
    typedef typename System::Kernel Kernel;
    
    typedef qi::rule<iterator, boost::shared_ptr<Constraint3D>(System*), qi::space_type > parser;
    static void init(parser& r) {
        r = qi::lit("<type>Constraint3D</type>")
	    >> ("<connect first=" >> qi::int_ >> "second=" >> qi::int_ >> "></connect>")[ 
		  qi::_val =  phx::construct<boost::shared_ptr<Constraint3D> >( 	
		      phx::new_<Constraint3D>(*qi::_r1, 
					      phx::bind(&System::Cluster::template getObject<Geometry3D, GlobalVertex>, phx::bind(&System::m_cluster, qi::_r1), qi::_1),
					      phx::bind(&System::Cluster::template getObject<Geometry3D, GlobalVertex>, phx::bind(&System::m_cluster, qi::_r1), qi::_2) ) )
		]
	    >> (*("<constraint type=" >> *qi_ascii::alpha >> ">" >> *qi_ascii::alnum >>"</constraint>"))[phx::bind(&details::setConstraints<Constraint3D>, qi::_1, qi::_val)];
    };
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::edge_prop, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::edge_prop, System, iterator > {
  
    typedef qi::rule<iterator, GlobalEdge(), qi::space_type> parser;
    static void init(parser& r) {
         r %= qi::lit("<type>Edge</type>")
	     >> "<value>" >> qi::int_ >> qi::int_ >> qi::int_ >> "</value>";
    };
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::fix_prop, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::fix_prop, System, iterator > {
  
    typedef qi::rule<iterator, bool(), qi::space_type> parser;
    static void init(parser& r) {
        r = qi::lit("<type>Fix</type>") >> "<value>" >> qi::bool_ >> "</value>";
    };
};

}


#endif //DCM_MODULE3D_STATE_HPP