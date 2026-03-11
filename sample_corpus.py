import requests
import json
from datetime import datetime
import os

def fetch_sample_documents():
    sources = {
        'stackoverflow': [],
        'wikipedia': [],
        'techcrunch': []
    }
    
    params = {
        'site': 'stackoverflow',
        'pagesize': 50,
        'page': 1,
        'order': 'desc',
        'sort': 'votes',
        'tagged': 'machine-learning;artificial-intelligence;python',
        'filter': '!*236ebekL1OIal5yiZrb)XZD0I1kIh*.-Y5KWNx'
    }
    
    response = requests.get('https://api.stackexchange.com/2.3/questions', params=params)
    data = response.json()
    
    for item in data.get('items', [])[:50]:
        doc = {
            'url': item.get('link', ''),
            'title': item.get('title', ''),
            'content': item.get('body', ''),
            'source': 'stackoverflow',
            'timestamp': item.get('creation_date', 0),
            'tags': item.get('tags', [])
        }
        sources['stackoverflow'].append(doc)
    
    wiki_params = {
        'action': 'query',
        'list': 'categorymembers',
        'cmtitle': 'Category:Artificial_intelligence',
        'cmlimit': 50,
        'format': 'json'
    }
    
    response = requests.get('https://en.wikipedia.org/w/api.php', params=wiki_params)
    data = response.json()
    
    for member in data['query']['categorymembers'][:50]:
        content_params = {
            'action': 'query',
            'prop': 'extracts',
            'titles': member['title'],
            'explaintext': True,
            'format': 'json'
        }
        
        response = requests.get('https://en.wikipedia.org/w/api.php', params=content_params)
        content_data = response.json()
        
        pages = content_data['query']['pages']
        page_data = list(pages.values())[0]
        
        doc = {
            'url': f"https://en.wikipedia.org/wiki/{member['title'].replace(' ', '_')}",
            'title': member['title'],
            'content': page_data.get('extract', ''),
            'source': 'wikipedia',
            'timestamp': int(datetime.now().timestamp()),
            'tags': ['encyclopedia', 'ai']
        }
        sources['wikipedia'].append(doc)
    
    import feedparser
    feed = feedparser.parse('https://techcrunch.com/feed/')
    
    for entry in feed.entries[:50]:
        doc = {
            'url': entry.get('link', ''),
            'title': entry.get('title', ''),
            'content': entry.get('summary', ''),
            'source': 'techcrunch',
            'timestamp': int(datetime.now().timestamp()),
            'tags': ['news', 'technology']
        }
        sources['techcrunch'].append(doc)
    
    return sources

def analyze_corpus(sources):
    total_docs = sum(len(docs) for docs in sources.values())
    total_raw_size = 0
    total_text_size = 0
    
    for source_name, docs in sources.items():
        for doc in docs:
            raw_size = len(json.dumps(doc))
            text_size = len(doc['content'])
            total_raw_size += raw_size
            total_text_size += text_size
    
    return {
        'total_documents': total_docs,
        'raw_size_bytes': total_raw_size,
        'raw_size_mb': total_raw_size / (1024 * 1024),
        'text_size_bytes': total_text_size,
        'text_size_mb': total_text_size / (1024 * 1024),
        'avg_doc_size': total_raw_size / total_docs if total_docs > 0 else 0,
        'avg_text_size': total_text_size / total_docs if total_docs > 0 else 0,
        'sources_count': len(sources),
        'sources': list(sources.keys())
    }

def save_corpus(sources, filename='sample_corpus.json'):
    all_docs = []
    for source_name, docs in sources.items():
        all_docs.extend(docs)
    
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(all_docs, f, ensure_ascii=False, indent=2)
    
    print(f"Corpus saved to {filename}")

def main():
    print("Fetching sample documents...")
    sources = fetch_sample_documents()
    
    print("\nAnalyzing corpus...")
    stats = analyze_corpus(sources)
    
    print("\nCorpus statistics:")
    print(f"Total documents: {stats['total_documents']}")
    print(f"Raw data size: {stats['raw_size_mb']:.2f} MB ({stats['raw_size_bytes']} bytes)")
    print(f"Extracted text size: {stats['text_size_mb']:.2f} MB ({stats['text_size_bytes']} bytes)")
    print(f"Average document size: {stats['avg_doc_size']:.0f} bytes")
    print(f"Average text size: {stats['avg_text_size']:.0f} bytes")
    print(f"Sources: {stats['sources_count']} ({', '.join(stats['sources'])})")
    
    save_corpus(sources)
    
    print("\nSample documents:")
    for source_name, docs in sources.items():
        if docs:
            print(f"\n{source_name.upper()}:")
            print(f"  URL: {docs[0]['url']}")
            print(f"  Title: {docs[0]['title'][:100]}...")
            print(f"  Content length: {len(docs[0]['content'])} chars")

if __name__ == '__main__':
    main()