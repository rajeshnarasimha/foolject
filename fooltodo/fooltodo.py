import os
import cgi
import wsgiref.handlers

from google.appengine.api import users
from google.appengine.api import images
from google.appengine.api import mail
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

class Greeting(db.Model):
  author = db.UserProperty()
  content = db.StringProperty(multiline=True)
  avatar = db.BlobProperty()
  date = db.DateTimeProperty(auto_now_add=True)

class MainPage(webapp.RequestHandler):
  def get(self):
    #greetings_query = Greeting.all().order('-date')
    #greetings = greetings_query.fetch(6)
    greetings = db.GqlQuery("Select * from Greeting Order by date DESC limit 6")
    #greetings = db.GqlQuery("Select * from Greeting where author=:author Order by date DESC limit 6", author=users.get_current_user())
    if users.get_current_user():
      url = users.create_logout_url(self.request.uri)
      url_linktext = 'Logout'
    else:
      url = users.create_login_url(self.request.uri)
      url_linktext = 'Login'  
    caption = "Fool To-do List"
    template_values = {
      'greetings': greetings,
      'url': url,
      'url_linktext': url_linktext,
      'caption': caption,
      }
    path = os.path.join(os.path.dirname(__file__), 'index.html')
    self.response.out.write(template.render(path, template_values))

class Guestbook(webapp.RequestHandler):
  def post(self):      
    greeting = Greeting()
    current_user = users.get_current_user()
    if current_user:
      greeting.author = current_user
    greeting.content = self.request.get('content')
    image = self.request.get("img")
    if image:
      avatar = images.resize(image, 32, 32)
      greeting.avatar = db.Blob(avatar)
    greeting.put()
    if current_user:        
      message = mail.EmailMessage()
      message.sender = current_user.email()
      message.to = "greatfoolbear@gmail.com"
      message.subject = "Message left in your guest book"
      message.body = """
 Dear Administrator,
 
 User %s Leave a message in your guest book. Please visit http://fooltodo.appspot.com to check it out.
 "%s"
 
 Fool To-do Team
 """ % (current_user.nickname(), greeting.content)
      message.html = """
 Dear Administrator,<br/><br/>
 
 User <b>%s</b> Leave a message in your guest book. Please visit <a href="http://fooltodo.appspot.com"><b>HERE</b></a> to check it out.<br/>
 <I>"%s"</I><br/><br/>
 
 Fool To-do Team
 """ % (current_user.nickname(), greeting.content)
      if image:
        message.attachments = [("avatar.jpg", greeting.avatar)]
      message.send()
    self.redirect('/')
    
class Image (webapp.RequestHandler):
  def get(self):
    greeting = db.get(self.request.get("img_id"))
    if greeting.avatar:
      self.response.headers['Content-Type'] = "image/jpeg"
      self.response.out.write(greeting.avatar)
    else:
      self.error(404)    

def main():
  application = webapp.WSGIApplication(
                                       [('/', MainPage),
                                        ('/img', Image),
                                        ('/sign', Guestbook)],
                                       debug=True)
  wsgiref.handlers.CGIHandler().run(application)

if __name__ == "__main__":
  main()