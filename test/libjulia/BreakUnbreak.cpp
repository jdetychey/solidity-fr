/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Unit tests that confirm that break and unbreak are inverse at least on most inputs.
 */

#include <test/libjulia/Common.h>

#include <libjulia/optimiser/NameCollector.h>
#include <libjulia/optimiser/ExpressionUnbreaker.h>
#include <libjulia/optimiser/ExpressionBreaker.h>

#include <libsolidity/inlineasm/AsmPrinter.h>

#include <boost/test/unit_test.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace std;
using namespace dev;
using namespace dev::julia;
using namespace dev::julia::test;
using namespace dev::solidity;


#define CHECK(_input)\
do\
{\
	auto result = parse(_input, false);\
	NameDispenser nameDispenser;\
	nameDispenser.m_usedNames = NameCollector(*result.first).names();\
	ExpressionBreaker{nameDispenser}(*result.first);\
	ExpressionUnbreaker un(*result.first);\
	un(*result.first);\
	BOOST_CHECK_EQUAL(assembly::AsmPrinter{}(*result.first), format(_input, false));\
}\
while(false)

BOOST_AUTO_TEST_SUITE(YulBreakUnbreak)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	CHECK("{ }");
}

BOOST_AUTO_TEST_CASE(control_flow)
{
	CHECK(R"({
		if mul(add(calldataload(0), 2), 3) {
			for { let a := 2 } lt(a, mload(a)) { a := add(a, mul(a, 2)) } {
				let b := mul(add(a, 2), 4)
				sstore(b, mul(b, 2))
			}
		}
	})");
}

BOOST_AUTO_TEST_CASE(functions)
{
	CHECK(R"({
		let x := f(0)
		function f(a) -> r {
			r := mload(mul(6, add(a, 0x20)))
		}
		for { let a := 2 } lt(a, mload(a)) { a := add(a, mul(a, 2)) } {
			let b := mul(add(a, f(a)), 4)
			sstore(b, mul(b, 2))
		}
	})");
}

BOOST_AUTO_TEST_SUITE_END()
