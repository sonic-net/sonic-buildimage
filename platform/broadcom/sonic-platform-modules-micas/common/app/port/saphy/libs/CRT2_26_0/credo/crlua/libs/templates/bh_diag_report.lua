return [===[
<div class="uk-container uk-container-xlarge">
    <article class="uk-article">
        <h1 class="uk-article-title">Slice {{slice_id}} Diagnostic Report</h1>
        <p class="uk-article-meta">Generated {{date}}</p>
        {% for _, dtable in ipairs(data) do %}
            <h2 class="uk-heading-line">{{dtable.title}}</h2>
            <pre><code class="language-plaintext">{{dtable.output}}</code></pre>
        {% end %}
    </article>
</div>
]===]
