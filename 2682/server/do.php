<html><head><link type="text/css" href="/css/test.css" rel="stylesheet"/></head>
<body><div class="header"><img src="img/test.png"/></div><h1>
<?php

// 显示所有接收到的参数
// echo "<h2>Received Parameters:</h2>";
// echo "<p>GET Parameters:</p><pre>" . print_r($_GET, true) . "</pre>";
echo "<p>POST Parameters:</p><pre>" . print_r($_POST, true) . "</pre>";

// if (isset($_GET["action"])) $action = $_GET["action"];
// else die("Missing action param</h1></body></html>");

// if ($action != "login") die("Not support action</h1></body></html>");

if (! isset($_POST["login"]) || ! isset($_POST["pass"])) 
      die("Login Failed (Missing login or pass param)</h1></body></html>");

if ($_POST["login"] == "3220102682" && $_POST["pass"] == "2682") {
      echo "User: ". $_POST["login"] . " Login OK<br/>";
      echo "Your IP: " . $_SERVER["REMOTE_ADDR"] ;
      echo ", Port: " . $_SERVER["REMOTE_PORT"]; 
}
else die("Login Failed</h1></body></html>");
?>
</h1>
</body></html>
