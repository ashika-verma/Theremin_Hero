$(function() {
  $("#logo").on("click", function(evt) {
    $("#home-tab").click();
  });

  $("#songs-body").on("click", "a", function(e) {
    $("#song-tab").show().click();
    var songName = $(e.target).data("name");
    console.log(songName);
    $.get("https://608dev.net/sandbox/sc/kgarner/project/server.py?songName=" + songName)
      .done(function(result) {
        console.log(result);
        $("#song-content").html(result);
      });
    console.log();
  });

  $("#windowTabs a.nav-link").on("shown.bs.tab", function(e) {
    if (e.target.id === "songs-tab") {
      $.get("https://608dev.net/sandbox/sc/kgarner/project/server.py")
        .done(function(result) {
          console.log(result);
          var html = $.map(result.split("\n"),
            function(line) {
              var part = line.split(",");
              return `<div><a href="#" data-name="${part[1]}">${part[1]}</a></div>`;
            }
          ).join("\n");
          $("#songs-body").html($(html));
        });
    } else if (e.target.id !== "song-tab") {
      $("#song-tab").hide();
    }
  });

  $("#song-upload").on("submit", function(evt) {
    var data = $("form").serializeArray().reduce(function(total, curr) {
      total[curr.name] = curr.value;
      return total;
    }, {});

    $.post("https://608dev.net/sandbox/sc/kgarner/project/server.py", data)
      .done(function(result) {
        $("#song-tab").show().click();
        $("#song-content").html(result);
      }).fail(function(err) {
        alert("Error: " + err);
      });

    evt.preventDefault();
    evt.stopPropagation();

    return false;
  });
})
