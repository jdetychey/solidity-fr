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
 * Optimiser component that uses the simplification rules to simplify expressions.
 */

#include <libjulia/optimiser/ExpressionSimplifier.h>

#include <libjulia/optimiser/SimplificationRules.h>
#include <libjulia/optimiser/ExpressionBreaker.h>
#include <libjulia/optimiser/ExpressionUnbreaker.h>
#include <libjulia/optimiser/NameCollector.h>
#include <libjulia/optimiser/NameDispenser.h>
#include <libjulia/optimiser/Semantics.h>
#include <libjulia/optimiser/SSAValueTracker.h>

#include <libsolidity/inlineasm/AsmData.h>

#include <libdevcore/CommonData.h>

using namespace std;
using namespace dev;
using namespace dev::julia;
using namespace dev::solidity;


void ExpressionSimplifier::visit(Expression& _expression)
{
	ASTModifier::visit(_expression);
	while (auto match = SimplificationRules::findFirstMatch(_expression, m_ssaValues))
	{
		// Do not apply the rule if it removes non-constant parts of the expression.
		// TODO: The check could actually be less strict than "movable".
		// We only require "Does not cause side-effects".
		// Note: Variable references are always movable, so if the current value
		// of the variable is not movable, the expression that references the variable still is.

		if (match->removesNonConstants && !MovableChecker(_expression).movable())
			return;
		_expression = match->action().toExpression(locationOf(_expression));
	}
}

void ExpressionSimplifier::run(Block& _ast)
{
	NameDispenser dispenser;
	dispenser.m_usedNames = NameCollector(_ast).names();

	// TODO we could assume it to be broken already
	ExpressionBreaker{dispenser}(_ast);

	SSAValueTracker ssaValues;
	ssaValues(_ast);
	ExpressionSimplifier{ssaValues.values()}(_ast);

	ExpressionUnbreaker{_ast}(_ast);
}
