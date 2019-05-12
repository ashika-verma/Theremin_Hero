import sqlite3
import datetime
import base64
import sys

sys.path.append('__HOME__/project')
from wav import write_song_from_string

song_db = "__HOME__/songs.db"

def read_song(name: str, id: int):
    """
    Reads the contents of a wave file corresponding to the song, "name"
    with a unique id, "id", and returns the contents, encoded in
    HTML

    Arguments:
        - name (str): the name of the song (may not be unique)
        - id (int): the unique id
    """
    song = open("__HOME__/" + name + "-" + str(id)  + ".ogg", "rb")
    base64_encoded = base64.encodebytes(song.read())

    return """
        <audio controls src="data:audio/ogg;base64,{}"/>
        """.format(base64_encoded.decode("utf-8"))

def handle_post(request):
    """
    Handles posting a new song to the server. Requries the following arguments:
        - songName (string): the name of the song. This could be a repeat
        - musicString: (string): a string with this format
            musicString ::= ( INT "," INT)*
            INT ::= [0-9]+

    If the arguments are valid, creates a new entry in the songs database
    """
    conn = sqlite3.connect(song_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS song_table (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                                                        songName TEXT, 
                                                        song TEXT, 
                                                        timing TIMESTAMP);''')

    songName = request['form']['songName']
    song = request['form']['musicString']
    id = 0

    try:
        if song.endswith(";"):
            song = song[:-1]
        c.execute('''INSERT into song_table VALUES (NULL,?,?,?);''', (songName,song,datetime.datetime.now()))
        id = c.lastrowid
        write_song_from_string(songName + "-" + str(id), song)
        conn.commit()
    except Exception as e:
        conn.rollback()
        raise e
    finally:
        conn.close()

    return read_song(songName, id)

def handle_get(request):
    """
    Handles retrieving a song for the ESP32 and website. Expects
    Has the following parameters:
        - id (optional string): the id of the song we wish to query
        - format (optional string): specifies whether to return the song in 
          an ESP32 format (the same format as POSTED) or HTML

    If no id is provided, returns a list of all the songs (per line) with the song
    name and id. Otherwise returns a specific song.
    """
    conn = sqlite3.connect(song_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS song_table (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                                                        songName TEXT, 
                                                        song TEXT, 
                                                        timing TIMESTAMP);''')

    if "id" in request["args"]:
        id = request["values"]["id"]
        dbSongs = c.execute('''SELECT songName,id,song FROM song_table WHERE id = ? ORDER BY timing ASC;''', (id,)).fetchall()

        if len(dbSongs) > 0:
            song = dbSongs[0]

            if request["values"].get("format") == "esp":
                return song[2]
            else:
                return read_song(song[0], song[1])
        return "ERROR: song not found"
    else:
        dbSongs = c.execute('''SELECT id,songName FROM song_table ORDER BY timing ASC;''').fetchall()

    songs = ''
    for song in dbSongs:
        songs += str(song[0]) + ',' + song[1] + '\n'

    conn.commit()
    conn.close()

    return songs

def request_handler(request):
    """
    A request handler for server/server.py
    """
    if request['method'] == 'POST':
        return handle_post(request)
    else:
        return handle_get(request)
