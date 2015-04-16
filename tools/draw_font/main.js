var color = "rgb(0,0,0)";
var backgroundColor = "rgb(255,255,255)";
var ascii = 0;
var table = [];
var font;

function Font(width, height, ascii)
{
    this.data = new Array(height);

    this.width = width;
    this.height = height;
    this.ascii = ascii;

    this.read_image = function(data, cell_size) {
        var cell_pitch = 4 * cell_size;
        var row_pitch = this.width * cell_pitch;
        for (var i = 0; i < this.height; ++i) {
            var num = 0;
            for (var j = 0; j < this.width; ++j) {
                var cell = data[(i * cell_size + cell_size / 2.0) * row_pitch + j * cell_pitch + cell_size / 2.0];
                if (cell == 0)
                    num += 1 << j;
            }
            this.data[i] = num;
        }
    };

    this.to_string = function() {
        var str = "[0x" + this.ascii.toString(16) + "] = { ";
        for (var i = 0; i < this.height; ++i) {
            str += "0x" + this.data[i].toString(16);
            if (i != this.height - 1)
                str += ", ";
        }
        str += " },";
        return str;
    }
}

function save()
{
    table.push(font.to_string());
    var str = "";
    for (var i = 0; i < table.length; ++i) {
        str += table[i] + "<br />";
    }
    $("#data").html(str);
}

function new_font()
{
    ascii = $("#char").val().charCodeAt(0);
    $("#code").html(ascii);
    setup();
}

function draw_grid(ctx, size, width, height)
{
    ctx.lineWidth = 1;

    for (var i = size; i < height; i += size) {
        if (i % (size * 2) == 0) {
            ctx.strokeStyle = '#666';
        } else {
            ctx.strokeStyle = '#aaa';
        }
        ctx.beginPath();
        ctx.moveTo(0, i);
        ctx.lineTo(width, i);
        ctx.stroke();
    }
    for (var i = size; i < width; i += size) {
        if (i % (size * 2) == 0) {
            ctx.strokeStyle = '#666';
        } else {
            ctx.strokeStyle = '#aaa';
        }
        ctx.beginPath();
        ctx.moveTo(i, 0);
        ctx.lineTo(i, height);
        ctx.stroke();
    }
}

function setup() 
{
    var grid_canvas = document.getElementById("grid");
    var ctx = grid_canvas.getContext("2d");

    var rows = parseInt($("#rows").val(), 10);
    var cols = parseInt($("#cols").val(), 10);
    var cell_size = parseInt($("#cell").val(), 10);

    grid_canvas.setAttribute("width", cols * cell_size);
    grid_canvas.setAttribute("height", rows * cell_size);

    ctx.clearRect(0, 0, cols * cell_size, rows * cell_size);
    ctx.fillStyle = backgroundColor;
    ctx.fillRect(0, 0, cols * cell_size, rows * cell_size);

    var xOffset = $("#grid").offset().left;
    var yOffset = $("#grid").offset().top;

    var mouseDown = false;
    var mouseButton = 0;

    $("#grid").mousedown(function(e) {
        mouseDown = true;
        mouseButton = e.which;
    }).bind('mouseup', function() {
        mouseDown = false;
    });

    font = new Font(cols, rows, ascii);

    draw_grid(ctx, cell_size, cols * cell_size, rows * cell_size);

    $("#grid").mousemove(function(e) {

        if (!mouseDown) return;

        var x = Math.floor((e.pageX-xOffset) / cell_size);
        var y = Math.floor((e.pageY-yOffset) / cell_size);

        ctx.fillStyle = mouseButton == 1 ? color : backgroundColor;
        ctx.fillRect(x * cell_size, y * cell_size, cell_size, cell_size);

        var data = ctx.getImageData(0, 0, cols * cell_size, rows * cell_size).data;

        font.read_image(data, cell_size);

        draw_grid(ctx, cell_size, cols * cell_size, rows * cell_size);
    });
}

window.onload = function() { 
    $("#cols").bind('keyup', function(e) {
        setup();
    });
    $("#rows").bind('keyup', function(e) {
        setup();
    });
    $("#cell").bind('keyup', function(e) {
        setup();
    });

    $("#char").bind('keyup', function(e) {
        new_font();
    });

    $("#grid").bind('contextmenu', function(e) {
        return false;
    });

    $("#save").click(function() {
        save();
    });

    $("#data").html("");

    setup();
}
