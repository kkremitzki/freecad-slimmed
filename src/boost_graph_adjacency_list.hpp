#ifndef FREECAD_ADJACENCY_LIST_HPP_WORKAROUND
#define FREECAD_ADJACENCY_LIST_HPP_WORKAROUND

// Workaround for boost >= 1.75
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/graph/adjacency_list.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS

#endif // #ifndef FREECAD_ADJACENCY_LIST_HPP_WORKAROUND
