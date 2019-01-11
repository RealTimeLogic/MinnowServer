<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>Minnow Server IoT Bridge</title>
<link rel="icon" href="MinnowServer.ico" type="image/x-icon" />
<link rel="stylesheet" type="text/css" href="css/theme.css" />
<link rel="stylesheet" type="text/css" href="css/grid.css" />
<link rel="stylesheet" type="text/css" href="css/menu.css" />
<link rel="stylesheet" type="text/css" href="css/credentials.css" />
</head>
<body>

  <!-- MENU MARKUP -->
    <div id="menu">

      <!-- MENU LOGO -->
        <a title="Powered by the awesome Minnow Server!" href="https://realtimelogic.com/products/sharkssl/minnow-server/" class="menu-logo">
          <img src="MinnowServer.png" />
        </a>
      <!-- END MENU LOGO -->

      <!-- DESKTOP LINKS -->
        <div id="desktop-menu" class="links-area desk">
          <h2>Minnow Server IoT Bridge</h2>
        </div>

    </div>
  <!-- END MENU MARKUP -->
<div class="content">
<div id="credentials-container" style="">
<div>
<?lsp

local function rmIP6(ip)
   if ip:find("::ffff:",1,true) == 1 then
      ip=ip:sub(8,-1)
   end
   return ip
end


local devs=app.getdevs()
local tid=request:data"connect"
if tid then
   local dev = devs[tonumber(tid)]
   if dev and not dev.usedby then -- Have device and not busy
      response:forward"device.htmls"
   end
   response:sendredirect"" -- ./
end
if next(devs) then
   response:write'<h1>Connected Devices</h1><ul>'
   for tid, dev in pairs(devs) do
      local li = dev.usedby and
         string.format('%s : %s, locked by: %s',dev.info, rmIP6(dev.peer),rmIP6(dev.usedby)) or
         string.format('<a href="./?connect=%s">%s</a> IP: %s',tid, dev.info, rmIP6(dev.peer))
      response:write('<li>',li,'</li>')
   end
   response:write'</ul>'
else ?>
        <h1>No Devices Connected</h1>
<?lsp end ?>
      </div>
    </div>

</div>
</body>
</html>
