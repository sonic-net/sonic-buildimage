#!/usr/bin/env python3

import re
import unittest


def _extract_version_from_content(content):
    """
    Extract SAI API version from header content.
    Returns the version string in format X.Y.Z or N/A if not available.
    """
    try:
        major_match = re.search(r'#define SAI_MAJOR\s+(\d+)', content)
        minor_match = re.search(r'#define SAI_MINOR\s+(\d+)', content)
        revision_match = re.search(r'#define SAI_REVISION\s+(\d+)', content)
        
        if major_match and minor_match and revision_match:
            major = major_match.group(1)
            minor = minor_match.group(1)
            revision = revision_match.group(1)
            return f"{major}.{minor}.{revision}"
        else:
            return "N/A"
            
    except Exception:
        return "N/A"


class TestSAIVersionExtraction(unittest.TestCase):
    
    def test_valid_sai_api_version(self):
        """Test extraction of valid SAI version"""
        content = """
        #define SAI_MAJOR 1
        #define SAI_MINOR 16
        #define SAI_REVISION 1
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "1.16.1")
    
    def test_missing_major(self):
        """Test handling of missing SAI_MAJOR"""
        content = """
        #define SAI_MINOR 16
        #define SAI_REVISION 1
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "N/A")
    
    def test_missing_minor(self):
        """Test handling of missing SAI_MINOR"""
        content = """
        #define SAI_MAJOR 1
        #define SAI_REVISION 1
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "N/A")
    
    def test_missing_revision(self):
        """Test handling of missing SAI_REVISION"""
        content = """
        #define SAI_MAJOR 1
        #define SAI_MINOR 16
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "N/A")
    
    def test_different_version_numbers(self):
        """Test extraction of different version numbers"""
        content = """
        #define SAI_MAJOR 2
        #define SAI_MINOR 5
        #define SAI_REVISION 10
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "2.5.10")
    
    def test_empty_content(self):
        """Test handling of empty content"""
        content = ""
        version = _extract_version_from_content(content)
        self.assertEqual(version, "N/A")
    
    def test_malformed_content(self):
        """Test handling of malformed content"""
        content = """
        #define SAI_MAJOR abc
        #define SAI_MINOR def
        #define SAI_REVISION ghi
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "N/A")
    
    def test_extract_version_from_content(self):
        """Test the helper function for extracting version from content"""
        content = """
        #define SAI_MAJOR 1
        #define SAI_MINOR 16
        #define SAI_REVISION 1
        """
        version = _extract_version_from_content(content)
        self.assertEqual(version, "1.16.1")
        
        content2 = """
        #define SAI_MAJOR 2
        #define SAI_MINOR 5
        #define SAI_REVISION 10
        """
        version2 = _extract_version_from_content(content2)
        self.assertEqual(version2, "2.5.10")
