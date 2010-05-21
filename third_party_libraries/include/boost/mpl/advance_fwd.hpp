
#ifndef BOOST_MPL_ADVANCE_FWD_HPP_INCLUDED
#define BOOST_MPL_ADVANCE_FWD_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: advance_fwd.hpp 49239 2008-10-10 09:10:26Z agurtovoy $
// $Date: 2008-10-10 11:10:26 +0200 (ven., 10 oct. 2008) $
// $Revision: 49239 $

#include <boost/mpl/aux_/common_name_wknd.hpp>

namespace boost { namespace mpl {

BOOST_MPL_AUX_COMMON_NAME_WKND(advance)

template< typename Tag > struct advance_impl;
template< typename Iterator, typename N > struct advance;

}}

#endif // BOOST_MPL_ADVANCE_FWD_HPP_INCLUDED
