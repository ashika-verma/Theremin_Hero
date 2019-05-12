import sqlite3
import datetime

song_db = "__HOME__/songs.db"

def handle_post(request):
    """
    Handles a POST request to server/score_server.py. Expects
    the following arguments:
        - userName (str): a string corresponding 
        - songId (int): the id of an existing song
        - score (int): the score for this song

    If the arguments are valid, creates a new score entry with 
    a name, userName, pointing to a song songId, with a score, score
    """
    conn = sqlite3.connect(song_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS score_table (songId INTEGER, 
                                                         userName TEXT, 
                                                         score INTEGER, 
                                                         timing TIMESTAMP, 
                                                         FOREIGN KEY(songId) REFERENCES song_table(id));''')
    userName = request['form']['userName']
    songId = int(request['form']['songId'])
    score = int(request['form']['score'])

    try:
        c.execute('''INSERT into score_table VALUES (?,?,?,?);''', 
                        (songId,userName,score,datetime.datetime.now()),)
        conn.commit()
    except Exception as e:
        conn.rollback()
        raise e
    finally:
        conn.close()

    return "SUCCESS"

def handle_get(request):
    """
    Handles retrieving score(s) for a song. If there is
    a userName, returns the highest score for the user 
    for the specified song. Otherwise, returns all the 

    Arguments:
        - songId (int): the id of the song we are searching
        - userName (optional string): the name of the user for
          whom we wish to get the highest score.
    """
    conn = sqlite3.connect(song_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS score_table (songId INTEGER, 
                                                         userName TEXT, 
                                                         score INTEGER, 
                                                         timing TIMESTAMP, 
                                                         FOREIGN KEY(songId) REFERENCES song_table(id));''')
    songId = int(request['values']['songId'])

    if 'userName' in request['args']:
        userName = request['values']['userName']
        try:
            userInfo = c.execute('''SELECT score,timing,MAX(score) FROM score_table WHERE userName = ? AND songId = ?;''', 
                                                                    (userName,songId)).fetchall()[0]
            return str(userInfo[0]) + "," + str(userInfo[1]) + "\n"
        except Exception as e:
            return "NO RESULTS FOUND"

    else:
        allScores = c.execute('''SELECT userName,score,timing FROM score_table WHERE songId = ? ORDER BY score DESC;''', 
                                                                (songId,)).fetchall()
        result = ""
        for score in allScores:
            result += str(score[0]) + "," + str(score[1]) + "," + str(score[2]) + "\n"
        return "NO RESULTS FOUND" if len(result) == 0 else result

def request_handler(request):
    """
    A general request handler for the score server
    """
    return handle_post(request) if request['method'] == 'POST' else handle_get(request)