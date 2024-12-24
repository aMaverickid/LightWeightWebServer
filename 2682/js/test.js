var int = self.setInterval("clock()",1000);
function clock()
{
    var d = new Date();
    var t = d.toLocaleTimeString();
    document.getElementById("clock").innerText = t;
}
