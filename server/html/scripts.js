$(function() {
  $("#song-tab").hide();

  var songId, songName, showScores = true;

  function showSong(name, html) {
    $("#song-name").text(name);
    $("#song-tab").show().click().text(name);
    $("#song-content").html(html);
  }

  $("#logo").on("click", function(evt) {
    $("#home-tab").click();
  });

  $("#songs-body").on("click", "a", function(e) {
    var target = $(e.target);
    var id = target.data("id");
    songName = target.text();
    var name = songName;

    $.get("https://608dev.net/sandbox/sc/kgarner/project/server.py?id=" + id)
      .done(function(result) {
        songId = id;
        showSong(name, result);
      });
  });

  $("#windowTabs a.nav-link").on("shown.bs.tab", function(e) {
    if (e.target.id !== "song-tab") {
      $("#song-tab").hide();
      $("#score-button").text("Show scores");
      $("#scores").html("").hide();

      songId = null;
      songName = null;
      showScores = true;
    }

    if (e.target.id === "songs-tab") {
      $.get("https://608dev.net/sandbox/sc/kgarner/project/server.py")
        .done(function(result) {
          var html = $.map(result.split("\n"),
            function(line) {
              var part = line.split(",");
              return `<div><a href="#" data-id="${part[0]}">${part[1]}</a></div>`;
            }
          ).join("\n");
          $("#songs-body").html($(html));
        });
    }
  });

  function getAllScores() {
    $.get("https://608dev.net/sandbox/sc/kgarner/project/score_server.py?songId=" + songId)
    .done(function(result) {
      var html;
      if (result.startsWith("NO RESULTS FOUND")) {
        html = `<div>No scores found for ${songName}</div>`;
      } else {
        html = $.map(result.split("\n"), function(line) {
          var content = line.split(",");
          return `<div><a class="score" href="#" data-user="${content[0]}">${content[0]}</a>: ${content[1]} at ${content[2]}</div>`;
        });
      }

      showScores = false;

      $("#score-button").text("Hide scores");
      $("#scores").html(html).show();
      $("#user-button").hide();
    }).fail(function(err) {
      alert("Error: " + err);
    });
  }

  $("#score-button").on("click", function(evt) {
    if (showScores) {
      console.log("getting scores");
      getAllScores();
    } else {
      showScores = true;
      $("#score-button").text("Show scores");
      $("#scores").html("").hide();
      $("#user-button").hide();
    }
  });

  $("#user-button").on("click", function(evt) {
    $(this).hide();
    getAllScores();
  });

  $("#scores").on("click", "a", function(evt) {
    var target = $(evt.target);
    var user = target.data("user");

    $.get(`https://608dev.net/sandbox/sc/kgarner/project/score_server.py?songId=${songId}&&userName=${user}`).done(function(result) {
      var html = "";
      if (result.startsWith("NO RESULTS FOUND")) {
        html = `<div>No scores found for ${songName}</div>`;
      } else {
        html = `<div>Score for: ${user}</div>` + $.map(result.split("\n"), function(line) {
          var content = line.split(",");
          return `<div>${content[0]} at ${content[1]}</div>`;
        });
      }

      $("#scores").html(html).show();
      $("#user-button").show();
    }).fail(function(err) {
      alert("Error: " + err);
    });
  });

  $("#song-upload").on("submit", function(evt) {
    var data = $("form").serializeArray().reduce(function(total, curr) {
      total[curr.name] = curr.value;
      return total;
    }, {});

    $.post("https://608dev.net/sandbox/sc/kgarner/project/server.py", data)
      .done(function(result) {
        showSong(data["songName"], result);
      }).fail(function(err) {
        alert("Error: " + err);
      });

    evt.preventDefault();
    evt.stopPropagation();

    return false;
  });
});
