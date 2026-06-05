return [==[
<!DOCTYPE html>
<html>
<head>
    <title>{*title or "Credo Report"*}</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <!-- UIkit CSS -->
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/uikit@3.14.3/dist/css/uikit.min.css" />
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.5.1/styles/monokai.min.css">

    <!-- UIkit JS -->
    <script src="https://cdn.jsdelivr.net/npm/uikit@3.14.3/dist/js/uikit.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/uikit@3.14.3/dist/js/uikit-icons.min.js"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.5.1/highlight.min.js"></script>
    <script src='https://cdn.plot.ly/plotly-2.12.1.min.js'></script>
    <script
        src="https://cdnjs.cloudflare.com/ajax/libs/highlightjs-line-numbers.js/2.8.0/highlightjs-line-numbers.min.js"
    >
    </script>

    <script>
        hljs.highlightAll();
        hljs.initLineNumbersOnLoad();
    </script>

    <style>
    /* for block of numbers */
    .hljs-ln .hljs-ln-numbers {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;

        text-align: right;
        color: #ccc;
        vertical-align: top;
        padding-right: 20px;

        /* your custom style here */
    }

    /* for block of code */
    .hljs-ln-code {
        padding-left: 10px;
    }
    </style>
</head>
<body>
    {*view*}
</body>
</html>
]==]
