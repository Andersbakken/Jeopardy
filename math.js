function multiply1(factor1, factor2)
{
    var ret = new Object;
    ret.question = factor1 + " * " + factor2 + " = ?";
    ret.answer = factor1 * factor2;
    return ret;
}

function multiply2(factor1, factor2)
{
    var ret = new Object;
    ret.question = factor1 + " * ? = " + (factor1 * factor2);
    ret.answer = factor2;
    return ret;
}

function division(factor1, factor2)
{
    var ret = new Object;
    ret.question = (factor1 * factor2) + " / " + factor2 + " = ?";
    ret.answer = factor1;
    return ret;
}

function extract(funcOrNumber, index)
{
    try {
        return funcOrNumber(index);
    } catch (err) {
        return funcOrNumber;
    }
}

function frames(f1min, f1max, f2min, f2max, func)
{
    var category = new Object;
    var questions = new Array;
    var answers = new Array;
    for (var j=0; j<5; ++j) {
        var frame = func(rand(extract(f1min, j), extract(f1max, j)),
                         rand(extract(f2min, j), extract(f2max, j)));
        questions[j] = frame.question;
        answers[j] = frame.answer;
    }
    category.questions = questions;
    category.answers = answers;
    return category;
}

function mult20(idx)
{
    return (idx + 1) * 20;
}

function mult5(idx)
{
    return (idx + 1) * 5;
}

function init()
{
    var categories = new Array;
    var category = frames(2, 10, 2, mult5, multiply1);
    category.topic = "Multiplication 1";
    categories[categories.length] = category;

    category = frames(2, 10, 10, mult20, multiply1);
    category.topic = "Multiplication 2";
    categories[categories.length] = category;

    category = frames(2, 10, 2, mult5, multiply2);
    category.topic = "Multiplication 3";
    categories[categories.length] = category;

    category = frames(2, 10, 10, mult20, multiply2);
    category.topic = "Multiplication 3";
    categories[categories.length] = category;

    category = frames(2, 10, 2, 10, division);
    category.topic = "Division 1";
    categories[categories.length] = category;

    category = frames(2, 10, 2, mult5, division);
    category.topic = "Division 2";
    categories[categories.length] = category;

    return categories;
}
