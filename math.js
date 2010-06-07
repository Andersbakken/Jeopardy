function init()
{
    var categories = new Array();
    for (var i=0; i<6; ++i) {
	var category = new Category();
	category["topic"] = i;
	var questions = new Array();
	var questions = new Array();
        for (var j=0; j<6; ++j) {
	    var val = (j + 1) * (i + 1);
	    questions[j] = val + ' * ' + val;
	    answers[j] = val * val;
	}
	category["questions"] = questions;
	category["answers"] = answers;
    }
    return categories;
}
