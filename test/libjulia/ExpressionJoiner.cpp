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
 * Unit tests for the expression joiner.
 */

#include <test/libjulia/Common.h>

#include <libjulia/optimiser/ExpressionJoiner.h>

#include <libsolidity/inlineasm/AsmPrinter.h>

#include <boost/test/unit_test.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace std;
using namespace dev;
using namespace dev::julia;
using namespace dev::julia::test;
using namespace dev::solidity;


#define CHECK(_original, _expectation)\
do\
{\
	auto result = parse(_original, false);\
	ExpressionJoiner::run(*result.first);\
	BOOST_CHECK_EQUAL(assembly::AsmPrinter{}(*result.first), format(_expectation, false));\
}\
while(false)

BOOST_AUTO_TEST_SUITE(YulExpressionJoiner)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	CHECK("{ }", "{ }");
}

BOOST_AUTO_TEST_CASE(simple)
{
	CHECK(
		R"({
			let a := mload(2)
			let x := calldataload(a)
			sstore(x, 3)
		})",
		R"({
			sstore(calldataload(mload(2)), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(multi)
{
	CHECK(
		R"({
			let a := mload(2)
			let b := mload(6)
			let x := mul(add(b, a), 2)
			sstore(x, 3)
		})",
		R"({
			sstore(mul(add(mload(6), mload(2)), 2), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(triple)
{
	CHECK(
		R"({
			let a := mload(2)
			let b := mload(6)
			let c := mload(7)
			let x := mul(add(c, b), a)
			sstore(x, 3)
		})",
		R"({
			sstore(mul(add(mload(7), mload(6)), mload(2)), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(multi_wrong_order)
{
	// We have an interleaved "add" here, so we cannot inline "a"
	// (note that this component does not analyze whether
	// functions are pure or not)
	CHECK(
		R"({
			let a := mload(2)
			let b := mload(6)
			let x := mul(a, add(2, b))
			sstore(x, 3)
		})",
		R"({
			let a := mload(2)
			sstore(mul(a, add(2, mload(6))), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(multi_wrong_order2)
{
	CHECK(
		R"({
			let a := mload(2)
			let b := mload(6)
			let x := mul(add(a, b), 2)
			sstore(x, 3)
		})",
		R"({
			let a := mload(2)
			sstore(mul(add(a, mload(6)), 2), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(multi_wrong_order3)
{
	CHECK(
		R"({
			let a := mload(3)
			let b := mload(6)
			let x := mul(add(b, a), mload(2))
			sstore(x, 3)
		})",
		R"({
			let a := mload(3)
			let b := mload(6)
			sstore(mul(add(b, a), mload(2)), 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(single_wrong_order)
{
	CHECK(
		R"({
			let a := mload(3)
			let b := sload(a)
			let c := mload(7)
			let d := add(b, c)
			sstore(d, 0)
		})",
		R"({
			let b := sload(mload(3))
			sstore(add(b, mload(7)), 0)
		})"
	);
}

BOOST_AUTO_TEST_CASE(reassignment)
{
	CHECK(
		R"({
			// This is not joined because a is referenced multiple times
			let a := mload(2)
			let b := mload(a)
			a := 4
		})",
		R"({
			let a := mload(2)
			let b := mload(a)
			a := 4
		})"
	);
}

BOOST_AUTO_TEST_CASE(multi_reference)
{
	CHECK(
		R"({
			// This is not joined because a is referenced multiple times
			let a := mload(2)
			let b := add(a, a)
		})",
		R"({
			let a := mload(2)
			let b := add(a, a)
		})"
	);
}

BOOST_AUTO_TEST_CASE(only_asignment)
{
	CHECK(
		R"({
			// This is not joined because a is referenced multiple times
			function f(a) -> x {
				a := mload(2)
				x := add(a, 3)
			}
		})",
		R"({
			function f(a) -> x
			{
				a := mload(2)
				x := add(a, 3)
			}
		})"
	);
}

BOOST_AUTO_TEST_CASE(if_condition)
{
	CHECK(
		R"({
			let a := mload(3)
			let b := sload(a)
			let c := mload(7)
			let d := add(c, b)
			if d {
				let x := mload(3)
				let y := add(x, 3)
			}
			let z := 3
			let t := add(z, 9)
		})",
		R"({
			if add(mload(7), sload(mload(3)))
			{
				let y := add(mload(3), 3)
			}
			let t := add(3, 9)
		})"
	);
}


BOOST_AUTO_TEST_CASE(switch_expression)
{
	CHECK(
		R"({
			let a := mload(3)
			let b := sload(a)
			let c := mload(7)
			let d := add(c, b)
			switch d
			case 3 {
				let x := mload(3)
				let y := add(x, 3)
			}
			default {
				sstore(1, 0)
			}
			let z := 3
			let t := add(z, 9)
		})",
		R"({
			switch add(mload(7), sload(mload(3)))
			case 3 {
				let y := add(mload(3), 3)
			}
			default {
				sstore(1, 0)
			}
			let t := add(3, 9)
		})"
	);
}

BOOST_AUTO_TEST_CASE(no_replacement_across_block)
{
	// The component will remove the empty block after
	// it has handled the outer block.
	// The idea behind this test is that the component
	// does not perform replacements across blocks because
	// they usually have contents, but adding contents
	// will reduce the scope of the test.
	CHECK(
		R"({
			let a := mload(2)
			let x := calldataload(a)
			{
			}
			sstore(x, 3)
		})",
		R"({
			let x := calldataload(mload(2))
			sstore(x, 3)
		})"
	);
}

BOOST_AUTO_TEST_CASE(no_replacement_in_loop_condition1)
{
	CHECK(
		R"({
			for { let b := mload(1) } b {} {}
		})",
		R"({
			for { let b := mload(1) } b {} {}
		})"
	);
}

BOOST_AUTO_TEST_CASE(no_replacement_in_loop_condition2)
{
	CHECK(
		R"({
			let a := mload(0)
			for { } a {} {}
		})",
		R"({
			let a := mload(0)
			for { } a {} {}
		})"
	);
}

BOOST_AUTO_TEST_SUITE_END()
