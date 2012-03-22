
var cat;

function CheckCharacters(text,allowed) {
  for (var i = 0; i < text.length; ++i) {
    if(allowed.indexOf(text.charAt(i))<0 ) {
      return false;
    }
  }
  return true;
}

function Calc() {
  var input = document.getElementById("input").value*1;
  if(CheckCharacters(input,"0123456789")) cat=input;

  for(var i=0;i<BIT_COUNT;++i)
    Mark(i);
}

function Mark(bit) {
  if((1<<bit) & cat) {
    if(!(bit%2))
      document.getElementById(PREFIX+bit).className="mark";
    else
      document.getElementById(PREFIX+bit).className="darkmark";
  }
  else {
    if(!(bit%2))
      document.getElementById(PREFIX+bit).className="";
    else
      document.getElementById(PREFIX+bit).className="dark";
  }
}

function Switch(bit) {
  cat = cat ^ (1 << bit);
  Mark(bit);
  document.getElementById("input").value = cat;
}