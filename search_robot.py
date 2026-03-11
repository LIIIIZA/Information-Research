import yaml
import requests
import feedparser
from pymongo import MongoClient
from datetime import datetime
import time
import hashlib
import logging
from urllib.parse import urlparse, urljoin
from bs4 import BeautifulSoup
import json

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filename='crawler.log'
)

class SearchRobot:
    def __init__(self, config_path):
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.client = MongoClient(
            self.config['db']['host'],
            self.config['db']['port']
        )
        self.db = self.client[self.config['db']['database']]
        self.collection = self.db[self.config['db']['collection']]
        
        self.collection.create_index('url', unique=True)
        self.collection.create_index('timestamp')
        self.collection.create_index('source')
        
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': self.config['logic']['user_agent']
        })
        
        self.stats = {
            'fetched': 0,
            'updated': 0,
            'errors': 0
        }
    
    def normalize_url(self, url):
        parsed = urlparse(url)
        return f"{parsed.scheme}://{parsed.netloc}{parsed.path}"
    
    def get_content_hash(self, content):
        return hashlib.md5(content.encode('utf-8')).hexdigest()
    
    def should_fetch(self, url, last_modified=None):
        existing = self.collection.find_one({'url': url})
        
        if existing is None:
            return True, None
        
        if 'content_hash' not in existing:
            return True, existing['_id']
        
        return False, existing['_id']
    
    def save_document(self, doc, doc_id=None):
        doc['crawled_at'] = int(datetime.now().timestamp())
        
        if doc_id:
            result = self.collection.update_one(
                {'_id': doc_id},
                {'$set': doc}
            )
            if result.modified_count > 0:
                self.stats['updated'] += 1
        else:
            try:
                self.collection.insert_one(doc)
                self.stats['fetched'] += 1
            except Exception as e:
                logging.error(f"Error saving document: {e}")
                self.stats['errors'] += 1
    
    def fetch_stackoverflow(self):
        if not self.config['sources']['stackoverflow']['enabled']:
            return
        
        source_config = self.config['sources']['stackoverflow']
        tags = '+'.join(source_config['tags'])
        
        for page in range(1, source_config['pages'] + 1):
            if self.stats['fetched'] + self.stats['updated'] >= self.config['logic']['max_documents']:
                break
            
            params = {
                'site': 'stackoverflow',
                'pagesize': self.config['logic']['batch_size'],
                'page': page,
                'order': 'desc',
                'sort': 'creation',
                'tagged': tags,
                'filter': '!*236ebekL1OIal5yiZrb)XZD0I1kIh*.-Y5KWNx'
            }
            
            try:
                response = self.session.get(
                    source_config['base_url'],
                    params=params,
                    timeout=self.config['logic']['timeout']
                )
                
                data = response.json()
                
                if 'items' not in 
                    break
                
                for item in data['items']:
                    url = self.normalize_url(item.get('link', ''))
                    
                    should_fetch, doc_id = self.should_fetch(url)
                    
                    if not should_fetch:
                        continue
                    
                    content = f"{item.get('title', '')} {item.get('body', '')}"
                    
                    doc = {
                        'url': url,
                        'raw_html': item.get('body', ''),
                        'source': 'stackoverflow',
                        'title': item.get('title', ''),
                        'tags': item.get('tags', []),
                        'content_hash': self.get_content_hash(content)
                    }
                    
                    self.save_document(doc, doc_id)
                    
                    time.sleep(self.config['logic']['delay_between_requests'])
                
                if not data.get('has_more', False):
                    break
                    
            except Exception as e:
                logging.error(f"Error fetching Stack Overflow page {page}: {e}")
                self.stats['errors'] += 1
                time.sleep(5)
    
    def fetch_wikipedia(self):
        if not self.config['sources']['wikipedia']['enabled']:
            return
        
        source_config = self.config['sources']['wikipedia']
        processed = set()
        
        for category in source_config['categories']:
            if len(processed) >= source_config['max_articles']:
                break
            
            params = {
                'action': 'query',
                'list': 'categorymembers',
                'cmtitle': f'Category:{category}',
                'cmlimit': 500,
                'format': 'json'
            }
            
            try:
                response = self.session.get(
                    source_config['base_url'],
                    params=params,
                    timeout=self.config['logic']['timeout']
                )
                
                data = response.json()
                
                for member in data['query']['categorymembers']:
                    if len(processed) >= source_config['max_articles']:
                        break
                    
                    title = member['title']
                    
                    if title in processed:
                        continue
                    
                    processed.add(title)
                    
                    content_params = {
                        'action': 'query',
                        'prop': 'revisions',
                        'titles': title,
                        'rvprop': 'content',
                        'format': 'json'
                    }
                    
                    response = self.session.get(
                        source_config['base_url'],
                        params=content_params
                    )
                    
                    content_data = response.json()
                    pages = content_data['query']['pages']
                    page_data = list(pages.values())[0]
                    
                    if 'revisions' not in page_
                        continue
                    
                    content = page_data['revisions'][0]['*']
                    url = f"https://en.wikipedia.org/wiki/{title.replace(' ', '_')}"
                    
                    should_fetch, doc_id = self.should_fetch(url)
                    
                    if not should_fetch:
                        continue
                    
                    soup = BeautifulSoup(content, 'html.parser')
                    text_content = soup.get_text()
                    
                    doc = {
                        'url': url,
                        'raw_html': content,
                        'source': 'wikipedia',
                        'title': title,
                        'category': category,
                        'content_hash': self.get_content_hash(text_content)
                    }
                    
                    self.save_document(doc, doc_id)
                    time.sleep(self.config['logic']['delay_between_requests'] / 2)
                
                time.sleep(1)
                
            except Exception as e:
                logging.error(f"Error fetching Wikipedia category {category}: {e}")
                self.stats['errors'] += 1
    
    def fetch_rss(self, source_name, rss_url, max_items):
        try:
            feed = feedparser.parse(rss_url)
            
            for entry in feed.entries[:max_items]:
                if self.stats['fetched'] + self.stats['updated'] >= self.config['logic']['max_documents']:
                    break
                
                url = self.normalize_url(entry.get('link', ''))
                
                should_fetch, doc_id = self.should_fetch(url)
                
                if not should_fetch:
                    continue
                
                content = f"{entry.get('title', '')} {entry.get('summary', '')}"
                
                doc = {
                    'url': url,
                    'raw_html': entry.get('content', [{}])[0].get('value', '') if hasattr(entry, 'content') else '',
                    'source': source_name,
                    'title': entry.get('title', ''),
                    'published': entry.get('published', ''),
                    'content_hash': self.get_content_hash(content)
                }
                
                self.save_document(doc, doc_id)
                time.sleep(self.config['logic']['delay_between_requests'])
                
        except Exception as e:
            logging.error(f"Error fetching RSS {source_name}: {e}")
            self.stats['errors'] += 1
    
    def run(self):
        print("Starting search robot...")
        logging.info("Search robot started")
        
        start_time = time.time()
        
        self.fetch_stackoverflow()
        print(f"Stack Overflow: {self.stats['fetched']} fetched, {self.stats['updated']} updated")
        
        self.fetch_wikipedia()
        print(f"Wikipedia: {self.stats['fetched']} fetched, {self.stats['updated']} updated")
        
        if self.config['sources'].get('techcrunch', {}).get('enabled', False):
            self.fetch_rss('techcrunch', 
                          self.config['sources']['techcrunch']['rss_url'],
                          self.config['sources']['techcrunch']['max_items'])
            print(f"TechCrunch: {self.stats['fetched']} fetched, {self.stats['updated']} updated")
        
        if self.config['sources'].get('hacker_news', {}).get('enabled', False):
            self.fetch_rss('hacker_news',
                          self.config['sources']['hacker_news']['rss_url'],
                          self.config['sources']['hacker_news']['max_items'])
            print(f"Hacker News: {self.stats['fetched']} fetched, {self.stats['updated']} updated")
        
        elapsed = time.time() - start_time
        
        print(f"\nTotal: {self.stats['fetched']} fetched, {self.stats['updated']} updated, {self.stats['errors']} errors")
        print(f"Time: {elapsed:.2f} seconds")
        
        logging.info(f"Robot finished: {self.stats}")

def main():
    robot = SearchRobot('config.yaml')
    robot.run()

if __name__ == '__main__':
    main()