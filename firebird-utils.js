function initLCD()
{
    var c = document.getElementById("canvas");

    var w = 320;
    var h = 240;
    c.width = w;
    c.height = h;

    var ctx = c.getContext('2d');
    var imageData = ctx.getImageData(0, 0, w, h);
    var bufSize = w * h * 4;
    var bufPtr = Module._malloc(bufSize);
    var buf = new Uint8Array(Module.HEAPU8.buffer, bufPtr, bufSize);

    var wrappedPaint = Module.cwrap('paintLCD', 'void', ['number']);
    repaint = function() {
        wrappedPaint(buf.byteOffset);
        imageData.data.set(buf);
        ctx.putImageData(imageData, 0, 0);
        window.requestAnimationFrame(repaint);
    };
    repaint();
}

function fileLoaded(event, filename)
{
    if (event.target.readyState == FileReader.DONE)
        FS.writeFile(filename, new Uint8Array(event.target.result), {encoding: 'binary'});
}

function fileLoad(event, filename)
{
    var file = event.target.files[0];

    if(!file)
        return FS.unlink(filename);

    var reader = new FileReader();
    reader.onloadend = function(event)
        {
            fileLoaded(event, filename);
        };

    if (file.webkitSlice)
        var blob = file.webkitSlice(0, file.size);
    else if (file.mozSlice)
        var blob = file.mozSlice(0, file.size);
    else
        var blob = file.slice(0, file.size);

    reader.readAsArrayBuffer(file);
}

function startEmulation()
{
    return Module.callMain();
}
