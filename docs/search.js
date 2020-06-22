(function() {
  'use strict'
  fetch('../index.json').then(r => r.json()).then(data => {
    var index = lunr.Index.load(data.index)
    var titles = data.titles

    var toc = document.getElementById('toc')
    var searchWrapper = document.createElement('div')
    searchWrapper.innerHTML = `
      Search: <input type="search">
      <ul class="results">
      </ul>
    `
    toc.insertBefore(searchWrapper, toc.querySelector('.contents'))
    var searchInput = searchWrapper.querySelector('input[type=search]')
    var resultsList = searchWrapper.querySelector('.results')
    searchInput.addEventListener('change', doSearch)

    // Allow specifying search query via query parameter.
    var m = window.top.location.search.match(/[?&]q=([^&]+)/)
    if (m) {
      searchInput.value = m[1]
      doSearch()
    }

    function doSearch() {
      var searchTerm = searchInput.value.trim()
      var html = ''
      if (searchTerm) {
        html = search(index, searchInput.value).map(result => `<li><a href='${result.ref}'>${titles[result.ref]}</a></li>`).join('\n')
        if (!html) html = `<li><em>No results</em></li>`
      }
      resultsList.innerHTML = html
    }
  })

  function search(index, searchTerm) {
    return index.query(query => {
      searchTerm.toLowerCase().split(/\s+/).forEach(term => {
        query.term(term, {
          fields: ["title"],
          boost: 10,
          wildcard: lunr.Query.wildcard.LEADING | lunr.Query.wildcard.TRAILING,
        })
        query.term(term, {
          fields: ["body"],
          editDistance: 1,
        })
      })
    })
  }
})()
