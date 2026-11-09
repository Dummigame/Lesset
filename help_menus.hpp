#include<imgui.h>
#include<string>
#include<random>

#ifndef LESSET
    #include"lesset.hpp"
#endif
#ifndef ROW_HEIGHT
#define ROW_HEIGHT 26
#endif



inline void displayCommonHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("√")) equation.append("√");
    ImGui::SetItemTooltip("Square root (sqrt) function.");

    if(ImGui::MenuItem("∛")) equation.append("∛");
    ImGui::SetItemTooltip("Cube root (cbrt) function.");

    if(ImGui::MenuItem("∜")) equation.append("∜");
    ImGui::SetItemTooltip("Quartic root (qtrt) function.");

    if(ImGui::MenuItem("root()")) equation.append("root(radicand, degree)");
    ImGui::SetItemTooltip("Nth root function, radicand on the left, degree right.\nMay be called with one argument for sqrt.");

    if(ImGui::MenuItem("ln")) equation.append("ln");
    ImGui::SetItemTooltip("Log with base ℯ.");

    if(ImGui::MenuItem("log()")) equation.append("log(base, value)");
    ImGui::SetItemTooltip("Generic log function, base on the left, expression right.\nMay be called with one argument for log10(expr).");

    if(ImGui::MenuItem("abs")) equation.append("abs");
    ImGui::SetItemTooltip("Absolute value as a function.");

    if(ImGui::MenuItem("sin")) equation.append("sin");
    ImGui::SetItemTooltip("Sine function.");

    if(ImGui::MenuItem("cos")) equation.append("cos");
    ImGui::SetItemTooltip("Cosine function.");

    if(ImGui::MenuItem("tan")) equation.append("tan");
    ImGui::SetItemTooltip("Tangent function.");

    if(ImGui::MenuItem("asin")) equation.append("asin");
    ImGui::SetItemTooltip("Arcsine function.");

    if(ImGui::MenuItem("acos")) equation.append("acos");
    ImGui::SetItemTooltip("Arccosine function.");

    if(ImGui::MenuItem("atan")) equation.append("atan");
    ImGui::SetItemTooltip("Arctangent function.");
    ImGui::EndMenu();
}

inline void displayTrigHelpMenu(std::string &equation, int &tooltipRnd)
{
    if(ImGui::MenuItem("sin")) equation.append("sin");
    ImGui::SetItemTooltip("Sine function.");

    if(ImGui::MenuItem("cos")) equation.append("cos");
    ImGui::SetItemTooltip("Cosine function.");

    if(ImGui::MenuItem("tan")) equation.append("tan");
    ImGui::SetItemTooltip("Tangent function.");

    if(ImGui::MenuItem("sec")) equation.append("sec");
    ImGui::SetItemTooltip("sec(x) = 1/cos(x)");

    if(ImGui::MenuItem("csc")) equation.append("csc");
    ImGui::SetItemTooltip("csc(x) = 1/sin(x).");

    if(ImGui::MenuItem("cot")) equation.append("cot");
    ImGui::SetItemTooltip("cot(x) = 1/tan(x).");

    if(ImGui::MenuItem("asin")) equation.append("asin");
    ImGui::SetItemTooltip("Arcsine function.");

    if(ImGui::MenuItem("acos")) equation.append("acos");
    ImGui::SetItemTooltip("Arccosine function.");

    if(ImGui::MenuItem("atan")) equation.append("atan");
    ImGui::SetItemTooltip("Arctangent function.");

    if(ImGui::MenuItem("atan2()")) equation.append("atan2(y,x)");
    ImGui::SetItemTooltip("Two-argument arctangent.");

    if(ImGui::MenuItem("asec")) equation.append("asec");
    ImGui::SetItemTooltip("Arcsecant function.");

    if(ImGui::MenuItem("acsc")) equation.append("acsc");
    ImGui::SetItemTooltip("Arccosecant function.");

    if(ImGui::MenuItem("acot")) equation.append("acot");
    ImGui::SetItemTooltip("Arccotangent function.");

    if(ImGui::MenuItem("sinc")) equation.append("sinc");
    ImGui::SetItemTooltip("sinc(x) = sin(x)/x /; x ≠ 0, sinc(0) = 1.");

    if(ImGui::MenuItem("cis")) equation.append("cis");
    if(ImGui::IsItemHovered())
    {
        std::uniform_int_distribution<> intDist(0, 1000);
        if(tooltipRnd == 0) tooltipRnd = intDist(lessetB::globals::randomMt);
        if(tooltipRnd != 1000) ImGui::SetItemTooltip("cis(x) = cos(x)+isin(x) = e^(ix).");
        else ImGui::SetItemTooltip("Something I'm not.");
    }
    else tooltipRnd = 0;
    if(ImGui::MenuItem("cas")) equation.append("cas");
    ImGui::SetItemTooltip("cas(x) = sin(x) + cos(x).");

    ImGui::EndMenu();
}

inline void displayHyperbolicHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("sinh")) equation.append("sinh");
    ImGui::SetItemTooltip("Hyperbolic sine function.");

    if(ImGui::MenuItem("cosh")) equation.append("cosh");
    ImGui::SetItemTooltip("Hyperbolic cosine function.");

    if(ImGui::MenuItem("tanh")) equation.append("tanh");
    ImGui::SetItemTooltip("Hyperbolic tangent function.");

    if(ImGui::MenuItem("sech")) equation.append("sech");
    ImGui::SetItemTooltip("Hyperbolic secant function.");

    if(ImGui::MenuItem("csch")) equation.append("csch");
    ImGui::SetItemTooltip("Hyperbolic cosecant function.");

    if(ImGui::MenuItem("coth")) equation.append("coth");
    ImGui::SetItemTooltip("Hyperbolic cotangent function.");

    if(ImGui::MenuItem("asinh")) equation.append("asinh");
    ImGui::SetItemTooltip("Hyperbolic arcsine function.");

    if(ImGui::MenuItem("acosh")) equation.append("acosh");
    ImGui::SetItemTooltip("Hyperbolic arccosine function.");

    if(ImGui::MenuItem("atanh")) equation.append("atanh");
    ImGui::SetItemTooltip("Hyperbolic arctangent function.");

    if(ImGui::MenuItem("asech")) equation.append("asech");
    ImGui::SetItemTooltip("Hyperbolic arcsecant function.");

    if(ImGui::MenuItem("acsch")) equation.append("acsch");
    ImGui::SetItemTooltip("Hyperbolic arccosecant function.");

    if(ImGui::MenuItem("acoth")) equation.append("acoth");
    ImGui::SetItemTooltip("Hyperbolic arccotangent function.");
    ImGui::EndMenu();
}

inline void displayCalculusHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("exp")) equation.append("exp");
    ImGui::SetItemTooltip("exp(x) = e^x");

    if(ImGui::MenuItem("ln")) equation.append("ln");
    ImGui::SetItemTooltip("Log with base ℯ.");

    if(ImGui::MenuItem("log()")) equation.append("log(base,value)");
    ImGui::SetItemTooltip("Generic log function, base on the left, expression right.\nMay be called with one argument for log10(expr).");

    if(ImGui::MenuItem("diff()")) equation.append("diff(expr)");
    ImGui::SetItemTooltip("Approximates a derivative. Takes one argument.");

    // if(ImGui::MenuItem("sum()")) equation.append("sum(expr, min, max)");
    // ImGui::SetItemTooltip("Like Σ. Argument 1 is an expression (which may contain n), Argument 2 is minimum n, Argument 3 is maximum n\nWarning:
    // this function is comically slow.");

    if(ImGui::MenuItem("lgamma")) equation.append("lgamma");
    ImGui::SetItemTooltip("Logarithm of gamma.");

    if(ImGui::MenuItem("gamma")) equation.append("gamma");
    ImGui::SetItemTooltip("Gamma.");

    ImGui::EndMenu();
}

inline void displayNumberTheoryHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("root()")) equation.append("root(denominator,enumerator)");
    ImGui::SetItemTooltip("Nth root function, denominator on the left, enumerator right.\nMay be called with one argument for sqrt.");

    if(ImGui::MenuItem("round")) equation.append("round");
    ImGui::SetItemTooltip("Rounds number to nearest integer.");

    if(ImGui::MenuItem("round()")) equation.append("round(expr, decimal places)");
    ImGui::SetItemTooltip("Rounds number to however many decimal places specified.");

    if(ImGui::MenuItem("bround")) equation.append("bround");
    ImGui::SetItemTooltip("Rounds number to nearest even integer (Banker's Rounding).");

    if(ImGui::MenuItem("floor")) equation.append("floor");
    ImGui::SetItemTooltip("Lowers number to next integer.");

    if(ImGui::MenuItem("ceil")) equation.append("ceil");
    ImGui::SetItemTooltip("Raises number to next integer.");

    if(ImGui::MenuItem("trunc")) equation.append("trunc");
    ImGui::SetItemTooltip("Truncates number to nearest integer closer to 0.");

    if(ImGui::MenuItem("trunc()")) equation.append("trunc(expr, decimal places)");
    ImGui::SetItemTooltip("Truncates number to however many decimal places specified.");

    if(ImGui::MenuItem("abs")) equation.append("abs");
    ImGui::SetItemTooltip("Absolute value as a function.");

    // if(ImGui::MenuItem("sabs()")) equation.append("sabs(x,λ)");
    // ImGui::SetItemTooltip("Absolute value, but smooth.\nArgument 1 is f(x), argument 2 λ (blending)");

    if(ImGui::MenuItem("sign")) equation.append("sign");
    ImGui::SetItemTooltip("Returns sign of input. N<0 => -1, 0 => 0, N>0 => 1");

    if(ImGui::MenuItem("gcf()")) equation.append("gcf(round )");
    ImGui::SetItemTooltip("Calculates the greatest common factor, takes multiple arguments.\nWill cause extreme output for numbers with many decimal "
                          "places.\nInputs will be rounded when graphing.");

    if(ImGui::MenuItem("lcm()")) equation.append("lcm(round ");
    ImGui::SetItemTooltip("Calculates the lowest common multiple, takes multiple arguments.\nWill cause extreme output for numbers with many decimal "
                          "places.\nInputs will be rounded when graphing.");

    if(ImGui::MenuItem("prime")) equation.append("prime");
    ImGui::SetItemTooltip("Returns 1 if a number is prime, else return 0.\nTry graphing this one, it's cool.");

    ImGui::EndMenu();
}

inline void displayStatisticsHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("rndint()")) equation.append("rndint(min, max)");
    ImGui::SetItemTooltip("Takes a lower bound and upper bound, returns a random integer in range.");

    if(ImGui::MenuItem("mean()")) equation.append("mean(");
    ImGui::SetItemTooltip("Averages inputs, takes multiple arguments.");

    if(ImGui::MenuItem("median()")) equation.append("median(");
    ImGui::SetItemTooltip("Evaluates all inputs and returns median, takes multiple arguments.\nWorks with only the real part of arguments.");

    if(ImGui::MenuItem("stdevp()")) equation.append("stdevp(");
    ImGui::SetItemTooltip("Population standard deviation, takes multiple arguments.");

    if(ImGui::MenuItem("max()")) equation.append("max(");
    ImGui::SetItemTooltip("Evaluates all inputs and returns greatest, takes multiple arguments.");

    if(ImGui::MenuItem("min()")) equation.append("min(");
    ImGui::SetItemTooltip("Evaluates all inputs and returns lowest, takes multiple arguments.");

    // if(ImGui::MenuItem("smax()")) equation.append("smax(f(x), g(x), λ)");
    // ImGui::SetItemTooltip("Smooth Maximum for 2 functions. Takes three arguments.\nArgument 1 is f(x), argument 2 g(x), argument 3 λ (blending)");

    // if(ImGui::MenuItem("smin()")) equation.append("smin(f(x), g(x), λ)");
    // ImGui::SetItemTooltip("Smooth Minimum for 2 functions. Takes three arguments.\nArgument 1 is f(x), argument 2 g(x), argument 3 λ (blending)");

    ImGui::EndMenu();
}

inline void displayComplexHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("real")) equation.append("real");
    ImGui::SetItemTooltip("Returns real part of input.");

    if(ImGui::MenuItem("imag")) equation.append("imag");
    ImGui::SetItemTooltip("Returns imaginary part of input as a real number.");

    if(ImGui::MenuItem("arg")) equation.append("arg");
    ImGui::SetItemTooltip("Counter-clockwise angle from positive real axis to line from origin to complex number.");

    if(ImGui::MenuItem("conj")) equation.append("conj");
    ImGui::SetItemTooltip("Flips sign of imaginary part of a number.");

    if(ImGui::MenuItem("norm")) equation.append("norm");
    ImGui::SetItemTooltip("Squares real and imaginary parts.");

    ImGui::EndMenu();
}

inline void displayMiscellaneousHelpMenu(std::string &equation)
{
    if(ImGui::MenuItem("inrange()")) equation.append("inrange()");
    ImGui::SetItemTooltip("inrange(min,expr,max) -> min<expr<max");

    if(ImGui::MenuItem("rndsel()")) equation.append("rndsel()");
    ImGui::SetItemTooltip("Evaluates all inputs and returns a random one, takes multiple arguments.");

    if(ImGui::MenuItem("fromHex()")) equation.append("fromHex()");
    ImGui::SetItemTooltip("Turns a hexadecimal number into decimal (as an unsigned integer).");

    if(ImGui::MenuItem("fromOct()")) equation.append("fromOct()");
    ImGui::SetItemTooltip("Turns an octal number into decimal (as an unsigned integer).");

    if(ImGui::MenuItem("fromBin()")) equation.append("fromHex()");
    ImGui::SetItemTooltip("Turns a binary number into decimal (as an unsigned integer).");

    if(ImGui::MenuItem("sat")) equation.append("sat");
    ImGui::SetItemTooltip("Linear transition between 0 and 1.");

    if(ImGui::MenuItem("sstep")) equation.append("sstep");
    ImGui::SetItemTooltip("Smooth transition between 0 and 1.");

    if(ImGui::MenuItem("mix()")) equation.append("mix(min, max, f(x)");
    ImGui::SetItemTooltip("Mixes args 1, 2, by a function between 0 and 1.");

    if(ImGui::MenuItem("ReLU")) equation.append("ReLU");
    ImGui::SetItemTooltip("if(f(x)>0), ReLU(f(x))=f(x), else ReLU(x)=0)");

    if(ImGui::MenuItem("fish")) equation.append("fish.");
    ImGui::SetItemTooltip("fishifies your equation.");

    ImGui::EndMenu();
}

inline void displayLogicHelpMenu(std::string &equation)
{
    ImGui::MenuItem("false = 0, true = 1, if N≠0 => true", NULL, false, false);

    if(ImGui::MenuItem("if()")) equation.append("if(condition, true, false)");
    ImGui::SetItemTooltip(
        "Argument 1 is a condition, argument 2 returns if true, argument 3 returns if false (optional).");

    if(ImGui::BeginMenu("Comparisons"))
    {
        if(ImGui::MenuItem("<")) equation.append("<");
        ImGui::SetItemTooltip("Returns true if left less than right.");

        if(ImGui::MenuItem(">")) equation.append(">");
        ImGui::SetItemTooltip("Returns true if left greater than right.");

        if(ImGui::MenuItem("≤")) equation.append("≤");
        ImGui::SetItemTooltip("Returns true if left less than or equal to right. (<=)");

        if(ImGui::MenuItem("≥")) equation.append("≥");
        ImGui::SetItemTooltip("Returns true if left greater than or equal to right. (>=)");

        if(ImGui::MenuItem("=")) equation.append("=");
        ImGui::SetItemTooltip("Returns true if left is exactly equal to right.\nThis can often be falsified by floating point precision.");

        if(ImGui::MenuItem("≠")) equation.append("≠");
        ImGui::SetItemTooltip("Returns true if left not equal to right. (=!)");

        if(ImGui::MenuItem("≈")) equation.append("≈");
        ImGui::SetItemTooltip("Returns true if left close to equal to right. (AROUND)\nCan be configured in options.\nUseful in graphing intersections, "
                            "zero points, so on.");

        if(ImGui::MenuItem("<=>")) equation.append("<=>");
        ImGui::SetItemTooltip("Spaceship operator. Returns -1 if left < right, 0 if left = right, 1 if left > right.");
        ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Logic"))
    {
        if(ImGui::MenuItem("¬")) equation.append("¬");
        ImGui::SetItemTooltip("Logical not, returns true if operand is false. (NOT)");

        if(ImGui::MenuItem("∨")) equation.append("∨");
        ImGui::SetItemTooltip("Logical or, returns true if either side is true. (OR)");

        if(ImGui::MenuItem("∧")) equation.append("∧");
        ImGui::SetItemTooltip("Logical and, returns true if both sides are true. (AND)");

        if(ImGui::MenuItem("⊕")) equation.append("⊕");
        ImGui::SetItemTooltip("Logical exclusive or, returns true if one side is true. (XOR)");
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Bitwise"))
    {
        if(ImGui::MenuItem("<<")) equation.append("<<");
        ImGui::SetItemTooltip("Left shift.");

        if(ImGui::MenuItem(">>")) equation.append(">>");
        ImGui::SetItemTooltip("Right shift.");

        if(ImGui::MenuItem("~")) equation.append("~");
        ImGui::SetItemTooltip("Bitwise not. (B!)");

        if(ImGui::MenuItem("B|")) equation.append("B|");
        ImGui::SetItemTooltip("Bitwise or.");

        if(ImGui::MenuItem("&")) equation.append("&");
        ImGui::SetItemTooltip("Bitwise and.");

        if(ImGui::MenuItem("^^")) equation.append("^^");
        ImGui::SetItemTooltip("Bitwise exclusive or.");
        ImGui::EndMenu();
    }
    ImGui::SetItemTooltip("Bitwise operators use int64_t under the hood.\nIf you don't know what this means,\nthese operators might not be too useful to you.");

    ImGui::EndMenu();
}

inline void displayScriptingHelpMenu()
{
    ImGui::Text("Scripting is a way to automate certain processes.");
    if(ImGui::BeginMenu("Commands"))
    {
        if(ImGui::BeginMenu("let"))
        {
            ImGui::Text("Let an expression be saved to a variable.\nMultiple assignments can be on one line.");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("graph"))
        {
            ImGui::Text("Add an equation to be graphed.\nUse numof before a variable to use its value.");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("fn"))
        {
            ImGui::Text("Define a custom function.\nExample: f(b,d) = b^2+sin d");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("op"))
        {
            ImGui::Text("Define a custom operator.\nSyntax: name(args)[associativity|postfix/prefix, precedence (0-34)]=expression.\nExample:\nop s^(b)[3,5]=b^b -> Defines postfix unary operator \"s^\"of b, returns b^b, has precedence of exponentiation, is right-associative");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("color"))
        {
            ImGui::Text("Change a graph's color (new color as hex code). Use indices.\nExample pattern:\ngraph x\ncolor 0 = #A4FFF8CC\nIndices are "
                        "offset based on if there's already graphs present.");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("IF"))
        {
            ImGui::Text("Run a block of commands only if an expression following 'IF' is true.\nEnd block with endIF, no nested IF statements are "
                        "supported.\nThis is not the same as if() in the calculator.");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("jump"))
        {
            ImGui::Text("Jump to any line in the script. Nothing bad will ever be caused by this statement.");
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Syntax"))
    {
        ImGui::Text("Scripts are almost like regular input line by line, except with extra commands.\nIt does not care about whitespace other than newlines.");
        ImGui::EndMenu();
    }
    ImGui::EndMenu();
}

inline void displayMathConstantsMenu(std::string &equation)
{
    if(ImGui::MenuItem("π")) equation.append("π");
    ImGui::SetItemTooltip("3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068");

    if(ImGui::MenuItem("ℯ")) equation.append("ℯ");
    ImGui::SetItemTooltip("2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427");

    if(ImGui::MenuItem("γ")) equation.append("γ");
    ImGui::SetItemTooltip("0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495");

    if(ImGui::MenuItem("φ")) equation.append("φ");
    ImGui::SetItemTooltip("1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137");

    if(ImGui::MenuItem("ξ")) equation.append("ξ");
    ImGui::SetItemTooltip("A random number between 0 and 1.");

    if(ImGui::MenuItem("rndint")) equation.append("rndint");
    ImGui::SetItemTooltip("A random positive integer.");

    if(ImGui::MenuItem("τ")) equation.append("τ");
    ImGui::SetItemTooltip("6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136");

    if(ImGui::MenuItem("∞")) equation.append("∞");
    ImGui::SetItemTooltip("Infinity... infinity... infinity... infinity... infinity...");

    if(ImGui::MenuItem("prc")) equation.append("prc");
    ImGui::SetItemTooltip("0.01");

    if(ImGui::MenuItem("ppm")) equation.append("ppm");
    ImGui::SetItemTooltip("1×10^-06");

    if(ImGui::MenuItem("ppb")) equation.append("ppb");
    ImGui::SetItemTooltip("1×10^-09");

    if(ImGui::MenuItem("ppt")) equation.append("ppt");
    ImGui::SetItemTooltip("1×10^-12");

    if(ImGui::MenuItem("dgr")) equation.append("dgr");
    ImGui::SetItemTooltip("0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482...\nDegrees to "
                          "Radiants. Try entering with numbers into trig functions.");

    if(ImGui::MenuItem("rad")) equation.append("rad");
    ImGui::SetItemTooltip("57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111095...");

    ImGui::EndMenu();
}

inline void displayPhysicsConstantsMenu(std::string &equation)
{
    if(ImGui::MenuItem("c")) equation.append("ec");
    ImGui::SetItemTooltip("Speed of light, 299792458");

    if(ImGui::MenuItem("ec")) equation.append("ec");
    ImGui::SetItemTooltip("Elementary charge, 1.602176634×10^-19");

    if(ImGui::MenuItem("G")) equation.append("G");
    ImGui::SetItemTooltip("Gravitational Constant, 6.6743×10^-11");

    if(ImGui::MenuItem("g")) equation.append("g");
    ImGui::SetItemTooltip("Gravity on Earth's surface, 9.80665");

    if(ImGui::MenuItem("α")) equation.append("α");
    ImGui::SetItemTooltip("0.0072973525693");

    if(ImGui::MenuItem("me")) equation.append("me");
    ImGui::SetItemTooltip("Mass of Earth, 5.9722×10^24");

    if(ImGui::MenuItem("H0")) equation.append("H0");
    ImGui::SetItemTooltip("2.2×10^-18");

    if(ImGui::MenuItem("Z0")) equation.append("Z0");
    ImGui::SetItemTooltip("376.730313668");

    if(ImGui::MenuItem("U0")) equation.append("U0");
    ImGui::SetItemTooltip("1.25663706212×10^-06");

    if(ImGui::MenuItem("E0")) equation.append("E0");
    ImGui::SetItemTooltip("8.8541878128×10^-12");

    if(ImGui::MenuItem("ma")) equation.append("ma");
    ImGui::SetItemTooltip("1.6605390666×10^-27");

    if(ImGui::MenuItem("R")) equation.append("R");
    ImGui::SetItemTooltip("8.31446261815");

    if(ImGui::MenuItem("Na")) equation.append("Na");
    ImGui::SetItemTooltip("6.02214076×10^23");

    if(ImGui::MenuItem("o")) equation.append("o");
    ImGui::SetItemTooltip("5.670374419×10^-08");

    if(ImGui::MenuItem("k")) equation.append("k");
    ImGui::SetItemTooltip("1.380649×10^-23");

    if(ImGui::MenuItem("a")) equation.append("a");
    ImGui::SetItemTooltip("0.0072973525693");

    if(ImGui::MenuItem("h")) equation.append("h");
    ImGui::SetItemTooltip("6.62607015×10^-34");

    ImGui::EndMenu();
}

inline void displayTechincalHelpMenu()
{
    if(ImGui::BeginMenu("Technical"))
    {

        if(ImGui::BeginMenu("Evaluation"))
        {
            ImGui::Text(
                "Input is evaluated in the following order (simplified):\n- Subexpressions/multi argument functions, 2 + {lcm(3,2)}\n\n- Functions, "
                "3{sin x}\n\n- "
                "Unary Operators, 2+{3!}\n\n- Exponentiation, 3{e^2}\n\n- Negation, 3/{- 2}\n\n- Implicit Multiplication, {3(2)}*3\n\tNote, this can "
                "be merged with explicit using an option\n\n- \"Multiplication\" (many included here), 2+{3*2}, 2+{3mod2}\n\n- Addition, {3 + -2} = "
                "1\n\tNote: Subtraction works as addition by a negative number\n\n- Comparisons, {3>2} AND true\n\n- Logical Operators, {true AND "
                "1}\n\nBesides exponentiation and single-argument functions, default operators are left-associative. Your own can be "
                "either.\nFunctions like round are the "
                "exception to this; if no\nsubexpression follows, like in sin 3x, one is formed on its own.\nsin 3x(2) -> sin(3x)*(2), sin -3x^2 - 2 "
                "-> sin(-3x^2) - 2, don't get\nconfused by complex numbers written as (real,imag)\n\nRight-associative operators are evaluated after "
                "left-associative ones in the same pass.");
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Instances"))
        {
            ImGui::Text(
                "An instance holds its own variables, sliders, operators and functions.\nIt is recommended to run a script in its own instance.");
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Scripting")) displayScriptingHelpMenu();
        if(ImGui::BeginMenu("Notes"))
        {
            ImGui::Text(
                "Variables have full precision, even if not all the digits are shown.\nFunctions and variables may share a name, using the name with "
                "parentheses following means you call the function!\nIn this case, it may be good to make your multiplication explicit.");
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}
