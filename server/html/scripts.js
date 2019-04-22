$(function() {
  $("#logo").on("click", function(evt) {
    $("#home-tab").click();
  });

  $("#windowTabs a.nav-link").on("shown.bs.tab", function(e) {
    if (e.target.id !== "song-tab") {
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
